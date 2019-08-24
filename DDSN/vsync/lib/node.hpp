/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef NDN_VSYNC_NODE_HPP_
#define NDN_VSYNC_NODE_HPP_

#include <functional>
#include <exception>
#include <map>
#include <deque>
#include <unordered_map>

#include "ndn-common.hpp"
#include "vsync-common.hpp"
#include "vsync-helper.hpp"
#include "recv-window.hpp"
#include "logging.hpp"
#include "odometer.hpp"

namespace ndn {
namespace vsync {

class Node {
public:
  /* Dependence injection: callback for application */
  using DataCb = std::function<void(const VersionVector& vv)>;
  using IsImportantData = std::function<bool(uint64_t node_id)>;

  /* typedef */
  enum DataType : uint32_t {
    kUserData       = 0,
    kGeoData        = 1,
    kRepoData       = 2,
    kSyncReply      = 9668,
    kConfigureInfo  = 9669,
    kVectorClock    = 9670
  };

  using GetCurrentPos = std::function<std::pair<double, double>()>;
  using GetNumSurroundingNodes = std::function<int()>;

  class Error : public std::exception {
   public:
    Error(const std::string& what) : what_(what) {}
    virtual const char* what() const noexcept override { return what_.c_str(); }
   private:
    std::string what_;
  };

  /* For user application */
  Node(Face &face,
       Scheduler &scheduler,
       KeyChain &key_chain,
       const NodeID &nid,
       const Name &prefix,
       DataCb on_data,
       IsImportantData is_important_data,
       GetCurrentPos getCurrentPos,
       GetNumSurroundingNodes getNumSurroundingNodes);

  void PublishData(const std::string& content, uint32_t type = kUserData);

private:
  /* Node properties */
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Face& face_;
  Scheduler& scheduler_;
  KeyChain& key_chain_;
  const NodeID nid_;            /* To be configured by application */
  Name prefix_;                 /* To be configured by application */
  std::random_device rdevice_;
  std::mt19937 rengine_;
  Logger logger;

  /* Node states */
  VersionVector version_vector_;
  VersionVector version_vector_data_;   /* Data interest already added */
  std::unordered_map<Name, std::shared_ptr<const Data>> data_store_;
  bool generate_data;           /* If false, PubishData() returns immediately */
  unsigned int notify_time;     /* No. of retx left for same sync interest */
  unsigned int left_retx_count; /* No. of retx left for same data interest */
  std::deque<Packet> pending_ack;           /* Multi-level queue */
  std::deque<Packet> pending_sync_interest;
  std::deque<Packet> pending_data_reply;
  std::deque<Packet> pending_data_interest;
  std::deque<Packet> inf_retx_data_interest;
  Name waiting_data;            /* Name of outstanding data interest from pending_interest queue */
  std::unordered_map<NodeID, EventId> one_hop;              /* Nodes within one-hop distance */
  std::unordered_map<Name, EventId> overheard_sync_interest;/* For sync ack suppression */
  std::unordered_map<NodeID, EventId> surrounding_producers;/* Soft state of interested producers of nearby nodes */
  bool is_static;               /* Static nodes don't generate data or log store */
  bool is_hibernate;            /* Soft state of whether there're no nodes around at this moment */
  size_t num_scheduler_retx;    /* Number of data interest the scheduler will put back to the queue (for statistics) */
  size_t num_scheduler_retx_inf;/* Number of data interest the scheduler will put back to the inf retx queue (for statistics) */
  size_t pending_forward;       /* Number of data interest in queue that will be forwarded */

  /* Constants */
  const int kInterestTransmissionTime = 1;  /* Times same data interest sent */
  const time::milliseconds kSendOutInterestLifetime = time::milliseconds(500);
  const time::milliseconds kRetxDataInterestTime = time::milliseconds(500);    // Period for re-insert to end of queue
  const time::milliseconds kInfRetxDataInterestTime = time::milliseconds(5000); // Period for re-insert to end of inf retx queue
  const time::milliseconds kAddToPitInterestLifetime = time::milliseconds(444);
  // const time::milliseconds kInterestWT = time::milliseconds(50);
  // const time::milliseconds kInterestWT = time::milliseconds(200);
  std::uniform_int_distribution<> packet_dist
    = std::uniform_int_distribution<>(10000, 15000);   /* microseconds */
  std::uniform_int_distribution<> hibernate_packet_dist_
    = std::uniform_int_distribution<>(1000000, 2000000);   /* microseconds */
  // Timeout to enter hibernate mode if no packet received
  const time::milliseconds kHibernateTime = time::milliseconds(1000);
  // Distributions for multi-hop
  std::uniform_int_distribution<> mhop_dist
    = std::uniform_int_distribution<>(0, 10000);
  int pMultihopForwardDataInterest = 5000;  // Non-const, different for repo
  // Distribution for data generation
  const int data_generation_rate_mean = 40000;
  // const int data_generation_rate_mean = 10000;
  std::poisson_distribution<> data_generation_dist
    = std::poisson_distribution<>(data_generation_rate_mean);
  // Threshold for bundled data fetching
  const size_t kMissingDataThreshold = 0x7fffffff;
  // MTU
  const size_t kMaxDataContent = 4000;
  // Delay for sending everything to avoid collision
  std::uniform_int_distribution<> dt_dist
    = std::uniform_int_distribution<>(0, 5000);
  // Delay for sending ACK when local vector is not newer
  std::uniform_int_distribution<> ack_dist
    // = std::uniform_int_distribution<>(5000, 10000);
    = std::uniform_int_distribution<>(20000, 40000);
  // Delay for sync interest retx
  // time::seconds kRetxTimer = time::seconds(2);
  // std::uniform_int_distribution<> retx_dist(2000000, 10000000);
  float retx_timer_base = 8000000;
  std::uniform_int_distribution<> retx_dist
    = std::uniform_int_distribution<>(retx_timer_base * 0.9, retx_timer_base * 1.1);
  // Delay for beacon frequency
  std::uniform_int_distribution<> beacon_dist
    = std::uniform_int_distribution<>(2000000, 3000000);
  const int kDataInterestRetries = 10;
  const int kInfRetxNum = 10000;   // Limit the number of inf retx data interests

  /* Options */
  /*const*/ bool kRetx =     true;        /* Use sync interest retx? */
  const bool kMultihopSync = true;        /* Use multihop for sync? */
  const bool kMultihopData = true;       /* Use multihop for data? */
  const bool kSyncAckSuppression = true;

  /* Callbacks */
  DataCb data_cb_;                      /* Never used in simulation */
  IsImportantData is_important_data_;   /* App decides whether to fetch data */
  GetCurrentPos getCurrentPos_;
  GetNumSurroundingNodes getNumSurroundingNodes_;
  Odometer odometer;

  /* Node statistics */
  // Sent
  unsigned int suppressed_sync_interest;  /* No of sync interest suppressed */
  unsigned int data_reply;                /* No of data sent */
  // Received
  unsigned int should_receive_sync_interest;   /* No of interest should received receiver side (for collision rate) */
  unsigned int received_sync_interest;    /* No of sync interest received */
  unsigned int received_data_interest;    /* No of data interest received */
  unsigned int received_data_mobile;      /* No of data received (if this node is mobile) */
  unsigned int received_data_mobile_from_repo;  /* No of data mobile nodes received from repo */
  int64_t hibernate_start;                /* Hibernation start time if in hibernation mode (micro-sec) */
  int64_t hibernate_duration;             /* Cumulative duration in hibernate mode (micro-sec) */

  /* Helper functions */
  void StartSimulation();
  void PrintNDNTraffic();
  void RemoveOldestInfInterest();
  std::deque<Packet>& GetQueueByType(const std::string &type);

  /* Packet processing pipeline */
  /* Unified queue for outgoing interest */
  void AsyncSendPacket();

  /* 1. Sync packet processing */
  void SendSyncInterest();
  void OnSyncInterest(const Interest &interest);
  void SendSyncAck(const Name &n);
  void OnSyncAck(const Data &ack);
  void OnNotifyDTTimeout();
  void OnNotifyWTTimeout();

  /* 2. Data packet processing */
  void SendDataInterest();
  void OnDataInterest(const Interest &interest);
  void SendDataReply();
  void OnDataReply(const Data &data, Packet::SourceType sourceType);
  EventId wt_data_interest; /* Event for sending next data interest */

  /* 3. Pro-active events (beacons and sync interest retx) */
  void RetxSyncInterest();
  void refreshHibernateTimer();
  EventId retx_event;       /* will send retx next sync intrest */
  EventId beacon_event;     /* will send retx next beacon */
  EventId packet_event;     /* Will send next packet async */
  EventId hibernate_event;  /* Will enter hibernate mode */
};

} // namespace vsync
} // namespace ndn

#endif  // NDN_VSYNC_NODE_HPP_
