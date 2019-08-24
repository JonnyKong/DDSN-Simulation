#include <random>

#undef NDEBUG

#include "node.hpp"
#include "vsync-helper.hpp"
#include "logging.hpp"

#include "ns3/simulator.h"
#include "ns3/nstime.h"

VSYNC_LOG_DEFINE(SyncForSleep);

namespace ndn {
namespace vsync {


/* Public */
Node::Node(Face &face, Scheduler &scheduler, KeyChain &key_chain, const NodeID &nid,
           const Name &prefix, DataCb on_data, IsImportantData is_important_data,
           GetCurrentPos getCurrentPos, GetNumSurroundingNodes getNumSurroundingNodes)
  : face_(face)
  , scheduler_(scheduler)
  , key_chain_(key_chain)
  , nid_(nid)
  , prefix_(prefix)
  , rengine_(rdevice_())
  , logger(nid_)
  , data_cb_(std::move(on_data))
  , is_important_data_(is_important_data)
  , getCurrentPos_(getCurrentPos)
  , getNumSurroundingNodes_(getNumSurroundingNodes)
  , odometer(getCurrentPos, face_.getIoService())
{

  /* Set interest filters */
  face_.setInterestFilter(
    Name(kSyncNotifyPrefix), std::bind(&Node::OnSyncInterest, this, _2),
    [this](const Name&, const std::string& reason) {
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Failed to register sync prefix: " << reason);
      throw Error("Failed to register sync interest prefix: " + reason);
  });
  face_.setInterestFilter(
    Name(kSyncDataPrefix), std::bind(&Node::OnDataInterest, this, _2),
    [this](const Name&, const std::string& reason) {
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Failed to register data prefix: " << reason);
      throw Error("Failed to register data prefix: " + reason);
  });

  /* Initialize statistics */
  received_sync_interest =          0;
  suppressed_sync_interest =        0;
  should_receive_sync_interest =    0;
  received_data_interest =          0;
  data_reply =                      0;
  received_data_mobile =            0;
  received_data_mobile_from_repo =  0;
  hibernate_start =                 0;
  hibernate_duration =              0;

  /* Initiate node states */
  is_static = false;
  generate_data = true;
  version_vector_[nid_] = 0;
  version_vector_data_[nid_] = 0;
  num_scheduler_retx = 0;
  num_scheduler_retx_inf = 0;
  pending_forward = 0;

  // if (nid_ >= 20) {
  // // if (nid_ == 1) {
  //   is_static = true;
  //   generate_data = false;
  //   pMultihopForwardDataInterest = 10000;
  // }

  // face1 = getFaceById_(1);

  /* Initiate event scheduling */
  /* 2s: Start simulation */
  scheduler_.scheduleEvent(time::milliseconds(2000), [this] { StartSimulation(); });

  /* 400s: Stop data generation */
  scheduler_.scheduleEvent(time::seconds(800), [this] {
    generate_data = false;
  });

  /* 1195s: Print NFD statistics */
  scheduler_.scheduleEvent(time::seconds(2395), [this] {
    // Repo nodes are also used to calculate collision rates
    std::cout << "node(" << nid_ << ") should_receive_sync_interest = " << should_receive_sync_interest << std::endl;
    std::cout << "node(" << nid_ << ") received_sync_interest = " << received_sync_interest << std::endl;

    if (!is_static) {
      std::cout << "node(" << nid_ << ") suppressed_sync_interest = " << suppressed_sync_interest << std::endl;
      std::cout << "node(" << nid_ << ") data_reply = " << data_reply << std::endl;
      std::cout << "node(" << nid_ << ") received_data_interest = " << received_data_interest << std::endl;
      std::cout << "node(" << nid_ << ") received_data_mobile = " << received_data_mobile << std::endl;
      std::cout << "node(" << nid_ << ") received_data_mobile_from_repo = " << received_data_mobile_from_repo << std::endl;

      if (is_hibernate)
        hibernate_duration += ns3::Simulator::Now().GetMicroSeconds() - hibernate_start;
      std::cout << "node(" << nid_ << ") hibernate_duration = " << (float)hibernate_duration / 1000000 << std::endl;
      PrintNDNTraffic();
    }
  });

  /* 1196s: Print Node statistics */
  scheduler_.scheduleEvent(time::seconds(2396), [this] {
    uint64_t seq_sum = 0;
    for (auto entry: version_vector_) {
      seq_sum += entry.second;
    }
    std::cout << "node(" << nid_ << ") seq sum: " << seq_sum << std::endl;
    // std::cout << "node(" << nid_ << ") seq = " << version_vector_[nid_] << std::endl;
  });
}

void Node::PublishData(const std::string& content, uint32_t type) {

  if (!generate_data) {
    return;
  }
  version_vector_[nid_]++;
  version_vector_data_[nid_]++;

  /* Make data name */
  auto n = MakeDataName(nid_, version_vector_[nid_]);
  std::shared_ptr<Data> data = std::make_shared<Data>(n);
  data->setFreshnessPeriod(time::seconds(3600));
  /* Set data content */
  proto::Content content_proto;
  // EncodeVV(version_vector_, content_proto.mutable_vv());
  EncodeVVWithInterest(version_vector_, content_proto.mutable_vv(), is_important_data_, surrounding_producers);
  content_proto.set_content(content);
  const std::string& content_proto_str = content_proto.SerializeAsString();
  data -> setContent(reinterpret_cast<const uint8_t*>(content_proto_str.data()),
                   content_proto_str.size());
  data -> setContentType(type);
  key_chain_.sign(*data, signingWithSha256());

  data_store_[n] = data;

  /* Print that both state and data have been stored */
  logger.logDataStore(n);
  logger.logStateStore(nid_, version_vector_[nid_]);
  VSYNC_LOG_TRACE( "node(" << nid_ << ") Publish Data: d.name=" << n.toUri() );

  /* Schedule next publish with same data */
  if (generate_data) {
    scheduler_.scheduleEvent(time::milliseconds(data_generation_dist(rengine_)),
                             [this, content] { PublishData(content); });
  } else {
    VSYNC_LOG_TRACE( "node(" << nid_ << ") Stopped data generation");
  }

  /* Send sync interest. No delay because callee already delayed randomly */
  SendSyncInterest();
}


/* Helper functions */
void Node::StartSimulation() {
  /* Init the first data publishing */
  std::string content = std::string(100, '*');
  scheduler_.scheduleEvent(time::milliseconds(100 * nid_),
                           [this, content] { PublishData(content); });

  /* Init async interest sending */
  scheduler_.cancelEvent(packet_event);
  packet_event = scheduler_.scheduleEvent(time::milliseconds(1 * nid_),
                                          [this] { AsyncSendPacket(); });
  /* Init hibernate timeout */
  is_hibernate = false;

  refreshHibernateTimer();
  odometer.init();

  if (kRetx) {
    int delay = retx_dist(rengine_);
    retx_event = scheduler_.scheduleEvent(time::microseconds(delay), [this] {
      RetxSyncInterest();
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Retx sync interest" );
    });
  }
}

void Node::PrintNDNTraffic() {
  /* Send a packet to trigger NFD to print */
  Interest i(kGetNDNTraffic, time::milliseconds(5));
  face_.expressInterest(i, [](const Interest&, const Data&) {},
                        [](const Interest&, const lp::Nack&) {},
                        [](const Interest&) {});
}

/**
 * Remove the oldest packet in the inf retx data interest queue
 */
void Node::RemoveOldestInfInterest() {
  int64_t oldest_timestamp_ms = INT64_MAX;
  std::deque<Packet>::iterator oldest_packet;
  std::deque<Packet>::iterator it = inf_retx_data_interest.begin();

  while (it != inf_retx_data_interest.end()) {
    if (it->inf_retx_start_time < oldest_timestamp_ms) {
      oldest_timestamp_ms = it->inf_retx_start_time;
      oldest_packet = it;
    }
    it++;
  }
  inf_retx_data_interest.erase(oldest_packet);
  VSYNC_LOG_TRACE( "node(" << nid_ << ") Removed an oldest interest from inf retx queue" );
}

/**
 * Given the type of packet, return the queue that the packet should be inserted into.
 */
std::deque<Packet>& Node::GetQueueByType(const std::string &type) {
  if (type == "SYNC_INTEREST")
    return pending_sync_interest;
  else if (type == "SYNC_REPLY")
    return pending_ack;
  else if (type == "DATA_INTEREST")
    return pending_data_interest;
  else if (type == "DATA_REPLY")
    return pending_data_reply;
  else if (type == "INF_RETX_DATA_INTEREST")
    return inf_retx_data_interest;
  else
    exit(1);
    // You shouldn't reach here.
}

void Node::AsyncSendPacket() {
  if (is_hibernate)
    SendSyncInterest();

  if (pending_sync_interest.size() > 0 ||
      pending_data_interest.size() > 0 ||
      pending_ack.size() > 0 ||
      pending_data_reply.size() > 0 || 
      inf_retx_data_interest.size() > 0) {

    /* Select the highest priority packet */
    Name n;
    Packet packet;
    bool is_inf_retx = false;
    if (pending_ack.size() > 0) {
      packet = pending_ack.front();
      pending_ack.pop_front();
      // pending_ack.clear();
    } else if (pending_data_reply.size() > 0) {
      packet = pending_data_reply.front();
      pending_data_reply.pop_front();
    } else if (pending_sync_interest.size() > 0) {
      packet = pending_sync_interest.front();
      pending_sync_interest.pop_front();
    } else if (pending_data_interest.size() > 0) {
      packet = pending_data_interest.front();
      pending_data_interest.pop_front();
    } else {
      packet = inf_retx_data_interest.front();
      inf_retx_data_interest.pop_front();
      is_inf_retx = true;
    }
    switch (packet.packet_type) {

      case Packet::INTEREST_TYPE:
        n = (packet.interest)->getName();
        if (n.compare(0, 2, kSyncDataPrefix) == 0) {            /* Data interest */
          /* Remove falsy data interest */
          if (data_store_.find(n) != data_store_.end()) {
            VSYNC_LOG_TRACE ("node(" << nid_ << ") Drop falsy data interest: i.name=" << n.toUri() );

            if (packet.packet_origin == Packet::FORWARDED)
              pending_forward--;
            VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length: " <<
                pending_data_interest.size() + inf_retx_data_interest.size() + 
                num_scheduler_retx - pending_forward );
            VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length inf: " <<
                             inf_retx_data_interest.size() + num_scheduler_retx_inf);
            AsyncSendPacket();
            return;
          }

          // If the node didn't travel far since last time sending this packet, put
          //  this data packet back into the queue, and send the next packet immediately.
          // To prevent iterating the queue too fast, need to add some delay.
          // if ((odometer.getDist() - packet.last_sent_dist < 50 ) && !packet.burst_packet) {
          //  if (odometer.getDist() - packet.last_sent_dist < 30) {
          // if (is_inf_retx && (odometer.getDist() - packet.last_sent_dist < 100)) {
          //   VSYNC_LOG_TRACE ("node(" << nid_ << ") Cancel data interest due to dist");
          //   scheduler_.scheduleEvent(time::seconds(1), [this, packet, is_inf_retx] {
          //     if (is_inf_retx)
          //       GetQueueByType("INF_RETX_DATA_INTEREST").push_back(packet);
          //     else
          //       GetQueueByType("DATA_INTEREST").push_back(packet);
          //   });
          //   AsyncSendPacket();
          //   return;
          // }
          
          face_.expressInterest(*packet.interest,
                                std::bind(&Node::OnDataReply, this, _2, packet.packet_origin),
                                [](const Interest&, const lp::Nack&) {},
                                [](const Interest&) {});
          int num_surrounding = getNumSurroundingNodes_();
          switch (packet.packet_origin) {
            case Packet::ORIGINAL:
              VSYNC_LOG_TRACE ("node(" << nid_ << ") Send Data Interest: i.name=" << n.toUri()
                                << ", should be received by " << num_surrounding );
              /* Add packet back to queue with longer delay to avoid retransmissions */
              if (--packet.nRetries >= 0 || is_inf_retx) {
                if (is_inf_retx)
                  num_scheduler_retx_inf++;
                num_scheduler_retx++;
                packet.last_sent_time = ns3::Simulator::Now().GetMicroSeconds();
                packet.last_sent_dist = odometer.getDist();
                packet.retransmission_counter ++;

                if (is_inf_retx) {
                  scheduler_.scheduleEvent(kInfRetxDataInterestTime, [this, packet] {
                    // inf_retx_data_interest.push_back(packet);
                    GetQueueByType("INF_RETX_DATA_INTEREST").push_back(packet);
                    num_scheduler_retx--;
                    num_scheduler_retx_inf--;
                  });
                }
                // else if (packet.nRetries % 3 == 0) {
                else if (1) {
                  packet.burst_packet = false;
                  scheduler_.scheduleEvent(kRetxDataInterestTime, [this, packet] {
                    // pending_data_interest.push_back(packet);
                    GetQueueByType("DATA_INTEREST").push_back(packet);
                    num_scheduler_retx--;
                  });
                } 
                else {
                  packet.burst_packet = true;
                  scheduler_.scheduleEvent(kSendOutInterestLifetime, [this, packet] {
                    // pending_data_interest.push_back(packet);
                    GetQueueByType("DATA_INTEREST").push_back(packet);
                    num_scheduler_retx--;
                  });
                }
              } else {
                // VSYNC_LOG_TRACE("DROP INTEREST");
                VSYNC_LOG_TRACE("APPEND INTEREST TO INF RETX QUEUE");
                packet.inf_retx_start_time = ns3::Simulator::Now().GetMicroSeconds();
                // inf_retx_data_interest.push_back(packet);
                GetQueueByType("INF_RETX_DATA_INTEREST").push_back(packet);
                while (inf_retx_data_interest.size() > uint32_t(kInfRetxNum))
                  RemoveOldestInfInterest();
              }
              break;
            case Packet::FORWARDED:
              VSYNC_LOG_TRACE ("node(" << nid_ << ") Send Data Interest (forward): i.name=" << n.toUri()
                                << ", should be received by " << num_surrounding );
              /* Best effort, don't add to queue */
              pending_forward--;
              break;
            default:
              assert(0);
          }
        }
        else if (n.compare(0, 2, kSyncNotifyPrefix) == 0) {   /* Sync interest */
          face_.expressInterest(*packet.interest,
                                std::bind(&Node::OnSyncAck, this, _2),
                                [](const Interest&, const lp::Nack&) {},
                                [](const Interest&) {});
          int num_surrounding = getNumSurroundingNodes_();
          VSYNC_LOG_TRACE ("node(" << nid_ << ") Send Sync Interest: i.name=" << n.toUri()
                           << ", should be received by " << num_surrounding );
          should_receive_sync_interest += num_surrounding;
        }
        break;

      case Packet::DATA_TYPE:
        n = (packet.data)->getName();
        if (n.compare(0, 2, kSyncDataPrefix) == 0) {            /* Data */
          data_reply++;
          face_.put(*packet.data);
          VSYNC_LOG_TRACE( "node(" << nid_ << ") Send Data Reply = " << (packet.data)->getName());
        } else if (n.compare(0, 2, kSyncNotifyPrefix) == 0) {   /* Sync ACK */
          VSYNC_LOG_TRACE ("node(" << nid_ << ") Send Sync Reply = " << n.toUri() );
          face_.put(*packet.data);
        } else {                                                /* Shouldn't get here */
          assert(0);
        }
        break;

      default:
        assert(0);  /* Shouldn't get here */
    }
  }

  /* Schedule self */
  int delay;
  if (!is_hibernate)
    delay = packet_dist(rengine_);
  else
    delay = hibernate_packet_dist_(rengine_);
  scheduler_.cancelEvent(packet_event);
  packet_event = scheduler_.scheduleEvent(time::microseconds(delay), [this] {
    AsyncSendPacket();
  });
}


/* Packet processing pipeline */
/* 1. Sync packet processing */
void Node::SendSyncInterest() {
  std::string encoded_vv = EncodeVVToNameWithInterest(version_vector_, is_important_data_, surrounding_producers);
  auto cur_time = ns3::Simulator::Now().GetMicroSeconds();
  auto pending_sync_notify = MakeSyncNotifyName(nid_, encoded_vv, cur_time);
  auto interest = std::make_shared<Interest>(pending_sync_notify, kSendOutInterestLifetime);
  interest->setMustBeFresh(true);
  Packet packet;
  packet.packet_type = Packet::INTEREST_TYPE;
  packet.interest = interest;
  pending_sync_interest.clear();
  // pending_sync_interest.push_back(packet);
  GetQueueByType("SYNC_INTEREST").push_back(packet);
}

void Node::OnSyncInterest(const Interest &interest) {

  const auto& n = interest.getName();
  // auto other_vv = DecodeVVFromName(ExtractEncodedVV(n));
  auto ret = DecodeVVFromNameWithInterest(ExtractEncodedVV(n));
  auto other_vv = ret.first;
  auto other_interested = ret.second;

  /* Update soft state of interested producers of nearby nodes */
  for (NodeID interested_node_id : other_interested) {
    auto it = surrounding_producers.find(interested_node_id);
    if (it != surrounding_producers.end())
      scheduler_.cancelEvent(it -> second);
    surrounding_producers[interested_node_id] = scheduler_.scheduleEvent(
      time::seconds(1),
      [this, interested_node_id] {
        surrounding_producers.erase(interested_node_id);
        VSYNC_LOG_TRACE ("node(" << nid_ << ") remove new surrounding producer: " << interested_node_id );
      }
    );
    VSYNC_LOG_TRACE ("node(" << nid_ << ") add new surrounding producer: " << interested_node_id );
  }

  received_sync_interest++;
  refreshHibernateTimer();

  /* Merge state vector, add missing data to pending_data_interest */
  bool other_vector_new = false;
  std::vector<Packet> missing_data;
  VersionVector mv;     /* For encoding bundled interest */
  for (auto entry : other_vv) {
    auto node_id = entry.first;
    auto seq_other = entry.second;
    if (version_vector_.find(node_id) == version_vector_.end() ||
        version_vector_[node_id] < seq_other) {
      auto start_seq = version_vector_.find(node_id) == version_vector_.end() ?
                       1: version_vector_[node_id] + 1;
      for (auto seq = start_seq; seq <= seq_other; ++seq)
        logger.logStateStore(node_id, seq);
      version_vector_[node_id] = seq_other;
    }

    // Don't add uninterested data interest to queue
    if (other_interested.find(node_id) == other_interested.end()) {
      // VSYNC_LOG_TRACE ("node(" << nid_ << ") jump over uninterested producer: " << node_id );
      // continue;
    }

    if (version_vector_data_.find(node_id) == version_vector_data_.end() ||
        version_vector_data_[node_id] < seq_other) {
      other_vector_new = true;
      auto start_seq = version_vector_data_.find(node_id) == version_vector_data_.end() ?
                       1: version_vector_data_[node_id] + 1;
      mv[node_id] = start_seq;
      for (auto seq = start_seq; seq <= seq_other; ++seq) {
        // logger.logStateStore(node_id, seq);
        if (is_important_data_(node_id) == false)  // Partial sync, skip data not interested
          continue;
        auto n = MakeDataName(node_id, seq);
        Packet packet;
        packet.packet_type = Packet::INTEREST_TYPE;
        packet.packet_origin = Packet::ORIGINAL;
        packet.last_sent_time = 0;
        packet.last_sent_dist = 0;
        packet.nRetries = kDataInterestRetries;
        packet.interest = std::make_shared<Interest>(n, kSendOutInterestLifetime);
        // const_cast<Interest*>(packet.interest.get())->setMustBeFresh(true);
        missing_data.push_back(packet);
      }
      version_vector_data_[node_id] = seq_other;
    }
  }
  for (size_t i = 0; i < missing_data.size(); ++i) {
    // pending_data_interest.push_back(missing_data[i]);
    GetQueueByType("DATA_INTEREST").push_back(missing_data[i]);
  }

  VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length: " <<
                   pending_data_interest.size() + inf_retx_data_interest.size() + 
                   num_scheduler_retx - pending_forward );
  VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length inf: " <<
                   inf_retx_data_interest.size() + num_scheduler_retx_inf);

  /* Do I have newer state? */
  /**
   * Check if I have newer state. Do not assume sync interests always carry
   *  entire vector. Assume my vector is newer only when I have a larger
   *  sequence number than a producer that exists in the vector.
   */
  bool my_vector_new = false;
  for (auto entry: version_vector_) {
    auto node_id = entry.first;
    auto seq = entry.second;
    // if (other_vv.find(node_id) == other_vv.end() ||
    //     other_vv[node_id] < seq) {
    if (other_vv.find(node_id) != other_vv.end() &&
        other_vv[node_id] < seq) {
      my_vector_new = true;
      break;
    }
  }


  /* If incoming state not newer, reset timer to delay sending next sync interest */
  if (!other_vector_new && !my_vector_new) {  /* Case 1: Other vector same  */
    scheduler_.cancelEvent(retx_event);
    int delay = retx_dist(rengine_);
    VSYNC_LOG_TRACE ("node(" << nid_ << ") Recv a syncNotify Interest:" << n.toUri()
                     << ", will reset retx timer" );
    pending_sync_interest.clear();
    retx_event = scheduler_.scheduleEvent(time::microseconds(delay), [this] {
      RetxSyncInterest();
    });
    suppressed_sync_interest++;
  } else if (!other_vector_new) {           /* Case 2: Other vector doesn't contain newer state */
    /* Do nothing */
    VSYNC_LOG_TRACE ("node(" << nid_ << ") Recv a syncNotify Interest:" << n.toUri()
                     << ", do nothing" );
  } else {                                /* Case 3: Other vector contain newer state */
    VSYNC_LOG_TRACE ("node(" << nid_ << ") Recv a syncNotify Interest:" << n.toUri()
                     << ", will do flooding");
    RetxSyncInterest(); // Flood
  }

  /**
   * If suppress ACK, always overhear other ACKs, otherwise always return ACK.
   * In both cases set ACK delay timer based on whether I have new state.
   */
  if (kSyncAckSuppression) {
    Interest interest_overhear(n, kAddToPitInterestLifetime);
    face_.expressInterest(interest_overhear, std::bind(&Node::OnSyncAck, this, _2),
                          [](const Interest&, const lp::Nack&) {},
                          [](const Interest&) {});
    auto p = overheard_sync_interest.find(n);
    if (p != overheard_sync_interest.end()) {
      scheduler_.cancelEvent(p -> second);  // Cancel to refresh event
      overheard_sync_interest.erase(p);
    }
    if (my_vector_new) {
      int delay = dt_dist(rengine_);
      overheard_sync_interest[n] = scheduler_.scheduleEvent(
        time::microseconds(delay), [this, n] {
          overheard_sync_interest.erase(n);
          VSYNC_LOG_TRACE ("node(" << nid_ << ") will reply ACK with new state: " << n.toUri() );
          SendSyncAck(n);
        }
      );
    }
    else {
      int delay = ack_dist(rengine_);
      VSYNC_LOG_TRACE ("node(" << nid_ << ") will reply ACK without new state: " << n.toUri() );
      overheard_sync_interest[n] = scheduler_.scheduleEvent(
        time::microseconds(delay), [this, n] {
          overheard_sync_interest.erase(n);
          SendSyncAck(n);
        }
      );
    }
  }
  else {
    int delay;
    if (my_vector_new) {
      delay = dt_dist(rengine_);
      VSYNC_LOG_TRACE ("node(" << nid_ << ") will reply ACK immediately:" << n.toUri() );
    } else {
      delay = ack_dist(rengine_);
      VSYNC_LOG_TRACE ("node(" << nid_ << ") will reply ACK with delay:" << n.toUri() );
    }
    scheduler_.scheduleEvent(time::microseconds(delay), [this, n] {
      SendSyncAck(n);
    });
  }
}

/* Append vector to name just before sending out ACK for freshness */
void Node::SendSyncAck(const Name &n) {
  std::shared_ptr<Data> ack = std::make_shared<Data>(n);
  proto::AckContent content_proto;
  // EncodeVV(version_vector_, content_proto.mutable_vv());
  EncodeVVWithInterest(version_vector_, content_proto.mutable_vv(), is_important_data_, surrounding_producers);
  const std::string& content_proto_str = content_proto.SerializeAsString();
  ack->setContent(reinterpret_cast<const uint8_t*>(content_proto_str.data()),
                  content_proto_str.size());
  ack->setFreshnessPeriod(time::milliseconds(1000));
  key_chain_.sign(*ack, signingWithSha256());

  Packet packet;
  packet.packet_type = Packet::DATA_TYPE;
  packet.data = ack;
  // pending_ack.push_back(packet);
  GetQueueByType("SYNC_REPLY").push_back(packet);
}

void Node::OnSyncAck(const Data &ack) {
  refreshHibernateTimer();

  const auto& n = ack.getName();
  if (kSyncAckSuppression){
    /* Remove pending ACK from both pending events and queue */
    for (auto it = pending_ack.begin(); it != pending_ack.end(); ++it) {
      if (it->data->getName() == n) {
        // it = pending_ack.erase(it);  // TODO: Cause bug for unknown reason
        // try { pending_ack.erase(it); } catch (...) {}
        pending_ack.clear();
        break;
      }
    }
    if (overheard_sync_interest.find(n) != overheard_sync_interest.end()) {
      scheduler_.cancelEvent(overheard_sync_interest[n]);
      overheard_sync_interest.erase(n);
      VSYNC_LOG_TRACE ("node(" << nid_ << ") Overhear sync ack, suppress pending ACK: " << ack.getName().toUri());
      return;
    }
  }

  VSYNC_LOG_TRACE ("node(" << nid_ << ") RECV sync ack: " << ack.getName().toUri());

  /* Extract difference and add to pending_data_interest */
  const auto& content = ack.getContent();
  proto::AckContent content_proto;
  if (!content_proto.ParseFromArray(content.value(), content.value_size())) {
    VSYNC_LOG_WARN("Invalid data AckContent format: nid=" << nid_);
    assert(false);
  }
  // auto vector_other = DecodeVV(content_proto.vv());
  auto ret = DecodeVVWithInterest(content_proto.vv());
  auto vector_other = ret.first;
  auto other_interested = ret.second;
  // bool other_vector_new = false;    /* Set if remote vector has newer state */
  std::vector<Packet> missing_data;
  for (auto entry : vector_other) {
    auto node_id = entry.first;
    auto node_seq = entry.second;

    if (version_vector_.find(node_id) == version_vector_.end() ||
        version_vector_[node_id] < node_seq) {
      auto start_seq = version_vector_.find(node_id) == version_vector_.end() ?
                       1: version_vector_[node_id] + 1;
      for (auto seq = start_seq; seq <= node_seq; ++seq)
        logger.logStateStore(node_id, seq);
      version_vector_[node_id] = node_seq;
    }

    // Don't add uninterested data interest to queue
    if (other_interested.find(node_id) == other_interested.end()) {
      // VSYNC_LOG_TRACE ("node(" << nid_ << ") jump over uninterested producer: " << node_id );
      // continue;
    }

    if (version_vector_data_.find(node_id) == version_vector_data_.end() ||
        version_vector_data_[node_id] < node_seq) {
      // other_vector_new = true;
      auto start_seq = version_vector_data_.find(node_id) == version_vector_data_.end() ?
                       1: version_vector_data_[node_id] + 1;
      for (auto seq = start_seq; seq <= node_seq; ++seq) {
        // logger.logStateStore(node_id, seq);
        if (is_important_data_(node_id) == false)  // Partial sync, skip data not interested
          continue;
        auto n = MakeDataName(node_id, seq);
        Packet packet;
        packet.packet_type = Packet::INTEREST_TYPE;
        packet.packet_origin = Packet::ORIGINAL;
        packet.nRetries = kDataInterestRetries;
        packet.last_sent_time = 0;
        packet.last_sent_dist = 0;
        packet.interest = std::make_shared<Interest>(n, kSendOutInterestLifetime);
        missing_data.push_back(packet);
      }
      version_vector_data_[node_id] = node_seq;
    }
  }
  for (size_t i = 0; i < missing_data.size(); ++i) {
    // pending_data_interest.push_back(missing_data[i]);
    GetQueueByType("DATA_INTEREST").push_back(missing_data[i]);
  }

  VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length: " <<
                   pending_data_interest.size() + inf_retx_data_interest.size() +
                   num_scheduler_retx - pending_forward );
  VSYNC_LOG_TRACE ("node(" << nid_ << ") Queue length inf: " <<
                   inf_retx_data_interest.size() + num_scheduler_retx_inf);
}


/* 2. Data packet processing */
void Node::SendDataInterest() {
  /* Do nothing */
}

void Node::OnDataInterest(const Interest &interest) {
  const auto& n = interest.getName();
  refreshHibernateTimer();
  VSYNC_LOG_TRACE( "node(" << nid_ << ") Recv data interest: i.name=" << n.toUri());

  auto iter = data_store_.find(n);
  if (iter != data_store_.end()) {
    Packet packet;
    packet.packet_type = Packet::DATA_TYPE;
    if (is_static) { /* Set content type for repos */
      Data data(n);
      data.setFreshnessPeriod(time::seconds(3600));
      data.setContent(iter->second->getContent().value(), iter->second->getContent().size());
      data.setContentType(kRepoData);
      key_chain_.sign(data, signingWithSha256());
      packet.data = std::make_shared<Data>(data);
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Send type repo data = " << iter->second->getName());
    } else {
      packet.data = iter -> second;
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Will send type regular data = " << iter->second->getName());
    }
    // pending_data_reply.push_back(packet);
    GetQueueByType("DATA_REPLY").push_back(packet);
  } else if (kMultihopData) {
    /* Otherwise add to my PIT, but send probabilistically */
    int p = mhop_dist(rengine_);
    if (p < pMultihopForwardDataInterest) {
      Packet packet;
      packet.packet_type = Packet::INTEREST_TYPE;
      packet.packet_origin = Packet::FORWARDED;
      packet.interest = std::make_shared<Interest>(n, kSendOutInterestLifetime);

      /**
       * Need to remove PIT, otherwise interferes with checking PIT entry.
       * Remove only in-record of wifi face, and out-record of app face.
       **/
      pending_data_interest.push_front(packet);
      pending_forward++;
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Add forwarded interest to queue: i.name=" << n.toUri());

    } else {
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Suppress data interest: i.name=" << n.toUri());
      Interest interest_suppress(n, kAddToPitInterestLifetime);
      face_.expressInterest(interest_suppress, std::bind(&Node::OnDataReply, this, _2, Packet::SUPPRESSED),
                            [](const Interest&, const lp::Nack&) {},
                            [](const Interest&) {});
    }
  }
}

void Node::SendDataReply() {
  /* Nothing */
  /* See Node::OnDataInterest() */
}

void Node::OnDataReply(const Data &data, Packet::SourceType sourceType) {

  refreshHibernateTimer();

  const auto& n = data.getName();
  NodeID node_id = ExtractNodeID(n);
  // uint64_t seq = ExtractSequence(n);
  if (data_store_.find(n) != data_store_.end()) {
    VSYNC_LOG_TRACE( "node(" << nid_ << ") Drops duplicate data: name=" << n.toUri());
    return;
  }

  /* Print based on source */
  switch(sourceType) {
    case Packet::ORIGINAL:
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Recv data reply: name=" << n.toUri());
      break;
    case Packet::FORWARDED:
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Recv forwarded data reply: name=" << n.toUri());
      break;
    case Packet::SUPPRESSED:
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Recv suppressed data reply: name=" << n.toUri());
      break;
    default:
      assert(0);
  }

  /**
   * Save data if this data is of interest. Only log important data stores to
   *  calculate data availability.
   */
  if (is_important_data_(node_id)) {
    /* Check content type */
    if (data.getContentType() == kRepoData) {
      std::shared_ptr<Data> data_no_flag = std::make_shared<Data>(n);
      data_no_flag -> setFreshnessPeriod(time::seconds(3600));
      data_no_flag -> setContent(data.getContent().value(), data.getContent().size());
      data_no_flag -> setContentType(kUserData);
      key_chain_.sign(*data_no_flag, signingWithSha256());
      data_store_[n] = data_no_flag;
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Receive data from repo: name=" << n.toUri());
      if (!is_static)
        received_data_mobile_from_repo++;
    } else {
      data_store_[n] = data.shared_from_this();
    }
    if (!is_static)
       received_data_mobile++;
    logger.logDataStore(n);
  }


  /**
   * Broadcast the received data for multi-hop only when it corresponds to
   *  the reply of a forwarded data interest.
   */
  if (kMultihopData && (sourceType == Packet::FORWARDED)) {
    Packet packet;
    packet.packet_type = Packet::DATA_TYPE;
    // if (!is_static) {
      packet.data = std::make_shared<Data>(data);
    //   VSYNC_LOG_TRACE( "node(" << nid_ << ") Re-broadcasting data reply: " << n.toUri() );
    // } else {
    //   /* Mark data as from repo */
    //   Data data_with_flag(n);
    //   data_with_flag.setFreshnessPeriod(time::seconds(3600));
    //   data_with_flag.setContent(data.getContent().value(), data.getContent().size());
    //   data_with_flag.setContentType(kRepoData);
    //   key_chain_.sign(data_with_flag, signingWithSha256());
    //   packet.data = std::make_shared<Data>(data_with_flag);
    //   VSYNC_LOG_TRACE( "node(" << nid_ << ") Re-broadcasting data reply: " << n.toUri() );
    // }
    // pending_data_reply.push_back(packet);
    GetQueueByType("DATA_REPLY").push_back(packet);
  }
}

/* 4. Pro-active events (beacons and sync interest retx) */
void Node::RetxSyncInterest() {
  SendSyncInterest();
  int delay = retx_dist(rengine_);
  scheduler_.cancelEvent(retx_event);
  retx_event = scheduler_.scheduleEvent(time::microseconds(delay), [this] {
    RetxSyncInterest();
    VSYNC_LOG_TRACE( "node(" << nid_ << ") Retx sync interest" );
  });
}


/**
 * Cancel pending hibernate event, and schedule a new one.
 *
 * In case is_hibernate is true before upon this function is called, this
 *  function also cancels and re-schedules AsyncSendPacket() event, in order to
 *  send next packet as soon as possible.
 */
void Node::refreshHibernateTimer() {
  if (is_static)
    return;
  if (is_hibernate) {
    VSYNC_LOG_TRACE( "node(" << nid_ << ") Leaves hibernate mode" );
    int delay = packet_dist(rengine_);
    scheduler_.cancelEvent(packet_event);
    packet_event = scheduler_.scheduleEvent(time::microseconds(delay), [this] {
      AsyncSendPacket();
    });
    is_hibernate = false;
    hibernate_duration += ns3::Simulator::Now().GetMicroSeconds() - hibernate_start;
  }
  scheduler_.cancelEvent(hibernate_event);
  hibernate_event = scheduler_.scheduleEvent(kHibernateTime, [this] {
    if (!is_hibernate) {
      hibernate_start = ns3::Simulator::Now().GetMicroSeconds();
      is_hibernate = true;
      VSYNC_LOG_TRACE( "node(" << nid_ << ") Enters hibernate mode due to timeout" );
    }
  });
}

} // namespace vsync
} // namespace ndn
