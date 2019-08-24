/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "forwarder.hpp"
#include "algorithm.hpp"
#include "core/logger.hpp"
#include "strategy.hpp"
#include "table/cleanup.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include "face/null-face.hpp"
#include <boost/random/uniform_int_distribution.hpp>
#include "core/random.hpp"

namespace nfd {

static inline double
getRandomNumber(double start, double end)
{
  std::uniform_real_distribution<double> dist(start, end);
  return dist(getGlobalRng());
}

static int kDataOut = 5000;
static const Name kSyncNotifyPrefix = Name("/ndn/syncNotify");
static const Name kSyncDataPrefix = Name("/ndn/vsyncData");
static const Name kBundledDataPrefix = Name("/ndn/bundledData");
static const Name kBeaconPrefix = Name("/ndn/beacon");
NFD_LOG_INIT("Forwarder");

Forwarder::Forwarder()
  : m_unsolicitedDataPolicy(new fw::DefaultUnsolicitedDataPolicy())
  , m_fib(m_nameTree)
  , m_pit(m_nameTree)
  , m_measurements(m_nameTree)
  , m_strategyChoice(m_nameTree, fw::makeDefaultStrategy(*this))
  , m_csFace(face::makeNullFace(FaceUri("contentstore://")))
{
  fw::installStrategies(*this);
  getFaceTable().addReserved(m_csFace, face::FACEID_CONTENT_STORE);

  m_faceTable.afterAdd.connect([this] (Face& face) {
    face.afterReceiveInterest.connect(
      [this, &face] (const Interest& interest) {
        this->startProcessInterest(face, interest);
      });
    face.afterReceiveData.connect(
      [this, &face] (const Data& data) {
        this->startProcessData(face, data);
      });
    face.afterReceiveNack.connect(
      [this, &face] (const lp::Nack& nack) {
        this->startProcessNack(face, nack);
      });
  });

  m_faceTable.beforeRemove.connect([this] (Face& face) {
    cleanupOnFaceRemoval(m_nameTree, m_fib, m_pit, face);
  });

  isSleep = false;
  in_data_dt = false;
  m_loss_rate = 0.0;

  m_outData = 0;
  m_outAck = 0;
  m_outBundledData = 0;
  m_outNotifyInterest = 0;
  m_outBeacon = 0;
  m_outBundledInterest = 0;
  m_outDataInterest = 0;
  m_cacheHit = 0;
  m_cacheHitSpecial = 0;
}

Forwarder::~Forwarder() = default;

void
Forwarder::startProcessInterest(Face& face, const Interest& interest)
{
  // check fields used by forwarding are well-formed
  try {
    if (interest.hasLink()) {
      interest.getLink();
    }
  }
  catch (const tlv::Error&) {
    NFD_LOG_DEBUG("startProcessInterest face=" << face.getId() <<
                  " interest=" << interest.getName() << " malformed");
    // It's safe to call interest.getName() because Name has been fully parsed
    return;
  }

  const Name getSyncTraffic = Name("/ndn/getNDNTraffic");
  if (interest.getName().compare(0, 2, getSyncTraffic) == 0) {
    // print the traffic info
    std::cout << "NFD: node(" << m_id << ") m_outNotifyInterest = " << m_outNotifyInterest << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_outDataInterest = " << m_outDataInterest << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_outBundledInterest = " << m_outBundledInterest << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_outBeacon = " << m_outBeacon << std::endl;

    std::cout << "NFD: node(" << m_id << ") m_outData = " << m_outData << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_outAck = " << m_outAck << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_outBundledData = " << m_outBundledData << std::endl;

    std::cout << "NFD: node(" << m_id << ") m_cacheHit = " << m_cacheHit << std::endl;
    std::cout << "NFD: node(" << m_id << ") m_cacheHitSpecial = " << m_cacheHitSpecial << std::endl;
    return;
  }

  /*
  // for heartbeat
  const Name heartbeat = Name("/ndn/heartbeat");
  if ((interest.getName().compare(0, 2, data_interest) == 0 ||
      interest.getName().compare(0, 2, sync_notify) == 0 ||
      interest.getName().compare(0, 2, heartbeat) == 0) &&
      face.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)
  {
    auto n = Name("/ndn/incomingPacketNotify");
    Interest i = Interest(n);
    auto& entry = m_fib.findLongestPrefixMatch(n);
    const fib::NextHopList& nexthops = entry.getNextHops();
    BOOST_ASSERT(nexthops.size() == 1);
    Face& appFace = nexthops.begin()->getFace();
    appFace.sendInterest(i);
  }
  */

  this->onIncomingInterest(face, interest);
}

void
Forwarder::startProcessData(Face& face, const Data& data)
{
  // check fields used by forwarding are well-formed
  // (none needed)

  /*
  // for heartbeat
  if ((data.getName().compare(0, 2, sync_data) == 0) &&
      face.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL)
  {
    auto n = Name("/ndn/incomingPacketNotify");
    Interest i = Interest(n);
    auto& entry = m_fib.findLongestPrefixMatch(n);
    const fib::NextHopList& nexthops = entry.getNextHops();
    BOOST_ASSERT(nexthops.size() == 1);
    Face& appFace = nexthops.begin()->getFace();
    appFace.sendInterest(i);
  }
  */

  this->onIncomingData(face, data);
}

void
Forwarder::startProcessNack(Face& face, const lp::Nack& nack)
{
  if (!isSleep) {
    // check fields used by forwarding are well-formed
    try {
      if (nack.getInterest().hasLink()) {
        nack.getInterest().getLink();
      }
    }
    catch (const tlv::Error&) {
      NFD_LOG_DEBUG("startProcessNack face=" << face.getId() <<
                    " nack=" << nack.getInterest().getName() <<
                    "~" << nack.getReason() << " malformed");
      return;
    }

    this->onIncomingNack(face, nack);
  }
}

void
Forwarder::onIncomingInterest(Face& inFace, const Interest& interest)
{
  // receive Interest
  NFD_LOG_DEBUG("onIncomingInterest face=" << inFace.getId() <<
                " interest=" << interest.getName());
  interest.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
  ++m_counters.nInInterests;

  // /localhost scope control
  bool isViolatingLocalhost = inFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                              scope_prefix::LOCALHOST.isPrefixOf(interest.getName());
  if (isViolatingLocalhost) {
    NFD_LOG_DEBUG("onIncomingInterest face=" << inFace.getId() <<
                  " interest=" << interest.getName() << " violates /localhost");
    // (drop)
    return;
  }

  // detect duplicate Nonce with Dead Nonce List
  bool hasDuplicateNonceInDnl = m_deadNonceList.has(interest.getName(), interest.getNonce());
  if (hasDuplicateNonceInDnl) {
    // goto Interest loop pipeline
    this->onInterestLoop(inFace, interest);
    return;
  }

  // PIT insert
  shared_ptr<pit::Entry> pitEntry = m_pit.insert(interest).first;

  // detect duplicate Nonce in PIT entry
  bool hasDuplicateNonceInPit = fw::findDuplicateNonce(*pitEntry, interest.getNonce(), inFace) !=
                                fw::DUPLICATE_NONCE_NONE;
  if (hasDuplicateNonceInPit) {
    // goto Interest loop pipeline
    this->onInterestLoop(inFace, interest);
    return;
  }

  // cancel unsatisfy & straggler timer
  this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

  if (interest.getName().compare(0, 2, kSyncDataPrefix) == 0) {
    if (interest.getInterestLifetime() == time::milliseconds(444)) {
      // insert in-record
      pitEntry->insertOrUpdateInRecord(const_cast<Face&>(inFace), interest);

      // set PIT unsatisfy timer
      this->setUnsatisfyTimer(pitEntry);
      // should recover
      // std::cout << "node(" << m_id << ") the current node only adds the pit entry, not sending the interests = " << interest.getName() << std::endl;
      return;
    }
  }

  const pit::InRecordCollection& inRecords = pitEntry->getInRecords();
  bool isPending = inRecords.begin() != inRecords.end();
  if (!isPending) {
    if (m_csFromNdnSim == nullptr) {
      m_cs.find(interest,
                bind(&Forwarder::onContentStoreHit, this, ref(inFace), pitEntry, _1, _2),
                bind(&Forwarder::onContentStoreMiss, this, ref(inFace), pitEntry, _1));
    }
    else {
      shared_ptr<Data> match = m_csFromNdnSim->Lookup(interest.shared_from_this());
      if (match != nullptr) {
        this->onContentStoreHit(inFace, pitEntry, interest, *match);
      }
      else {
        NFD_LOG_DEBUG("entering onContentMiss()");
        this->onContentStoreMiss(inFace, pitEntry, interest);
      }
    }
  }
  else {
    NFD_LOG_DEBUG("entering onContentMiss()");
    this->onContentStoreMiss(inFace, pitEntry, interest);
  }
}

void
Forwarder::onInterestLoop(Face& inFace, const Interest& interest)
{
  // if multi-access face, drop
  if (inFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) {
    NFD_LOG_DEBUG("onInterestLoop face=" << inFace.getId() <<
                  " interest=" << interest.getName() <<
                  " drop");
    return;
  }

  NFD_LOG_DEBUG("onInterestLoop face=" << inFace.getId() <<
                " interest=" << interest.getName() <<
                " send-Nack-duplicate");

  // send Nack with reason=DUPLICATE
  // note: Don't enter outgoing Nack pipeline because it needs an in-record.
  lp::Nack nack(interest);
  nack.setReason(lp::NackReason::DUPLICATE);
  inFace.sendNack(nack);
}

void
Forwarder::onContentStoreMiss(const Face& inFace, const shared_ptr<pit::Entry>& pitEntry,
                              const Interest& interest)
{
  NFD_LOG_DEBUG("onContentStoreMiss interest=" << interest.getName());

  // insert in-record
  pitEntry->insertOrUpdateInRecord(const_cast<Face&>(inFace), interest);

  // set PIT unsatisfy timer
  this->setUnsatisfyTimer(pitEntry);

  // has NextHopFaceId?
  shared_ptr<lp::NextHopFaceIdTag> nextHopTag = interest.getTag<lp::NextHopFaceIdTag>();
  if (nextHopTag != nullptr) {
    // chosen NextHop face exists?
    Face* nextHopFace = m_faceTable.get(*nextHopTag);
    if (nextHopFace != nullptr) {
      NFD_LOG_DEBUG("onContentStoreMiss interest=" << interest.getName() << " nexthop-faceid=" << nextHopFace->getId());
      // go to outgoing Interest pipeline
      // scope control is unnecessary, because privileged app explicitly wants to forward
      this->onOutgoingInterest(pitEntry, *nextHopFace, interest);
    }
    return;
  }

  // dispatch to strategy: after incoming Interest
  this->dispatchToStrategy(*pitEntry,
    [&] (fw::Strategy& strategy) { strategy.afterReceiveInterest(inFace, interest, pitEntry); });
}

void
Forwarder::onContentStoreHit(const Face& inFace, const shared_ptr<pit::Entry>& pitEntry,
                             const Interest& interest, const Data& data)
{
  NFD_LOG_DEBUG("onContentStoreHit interest=" << interest.getName());

  beforeSatisfyInterest(*pitEntry, *m_csFace, data);
  this->dispatchToStrategy(*pitEntry,
    [&] (fw::Strategy& strategy) { strategy.beforeSatisfyInterest(pitEntry, *m_csFace, data); });

  data.setTag(make_shared<lp::IncomingFaceIdTag>(face::FACEID_CONTENT_STORE));
  // XXX should we lookup PIT for other Interests that also match csMatch?

  // set PIT straggler timer
  this->setStragglerTimer(pitEntry, true, data.getFreshnessPeriod());

  // goto outgoing Data pipeline
  this->onOutgoingData(data, *const_pointer_cast<Face>(inFace.shared_from_this()));

  // do statistics
  if (data.getName().compare(0, 2, kSyncDataPrefix) == 0) {
    m_cacheHit++;
    if (inFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
      m_cacheHitSpecial++;
    }
  }
}

void
Forwarder::onOutgoingInterest(const shared_ptr<pit::Entry>& pitEntry, Face& outFace, const Interest& interest)
{
  NFD_LOG_DEBUG("onOutgoingInterest face=" << outFace.getId() <<
                " interest=" << pitEntry->getName());

  // insert out-record
  pitEntry->insertOrUpdateOutRecord(outFace, interest);
  if (interest.getName().compare(0, 2, kSyncDataPrefix) == 0 || 
      interest.getName().compare(0, 2, kSyncNotifyPrefix) == 0) {
    if (interest.getInterestLifetime() == time::milliseconds(444)) {
      return;
    }
  }

  // record related sync interests
  if (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
      interest.getInterestLifetime() != time::milliseconds(444)) {
    if (interest.getName().compare(0, 2, kSyncDataPrefix) == 0) m_outDataInterest++;
    else if (interest.getName().compare(0, 2, kBundledDataPrefix) == 0) m_outBundledInterest++;
    else if (interest.getName().compare(0, 2, kSyncNotifyPrefix) == 0) m_outNotifyInterest++;
    else if (interest.getName().compare(0, 2, kBeaconPrefix) == 0) m_outBeacon++;
  }

  // simulate packet loss at the sender side
  if ((interest.getName().compare(0, 2, kSyncNotifyPrefix) == 0 ||
      interest.getName().compare(0, 2, kSyncDataPrefix) == 0 ||
      interest.getName().compare(0, 2, kBundledDataPrefix) == 0 ||
      interest.getName().compare(0, 2, kBeaconPrefix) == 0) &&
      outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
      m_loss_rate != 0.0)
  {
    uint64_t number = getRandomNumber(0, 100);
    double bound = m_loss_rate * 100;
    if (number >= 0 && number < bound) {
      return;
    }
  }

  // send Interest
  outFace.sendInterest(interest);
  ++m_counters.nOutInterests;
}

void
Forwarder::onInterestReject(const shared_ptr<pit::Entry>& pitEntry)
{
  if (fw::hasPendingOutRecords(*pitEntry)) {
    NFD_LOG_ERROR("onInterestReject interest=" << pitEntry->getName() <<
                  " cannot reject forwarded Interest");
    return;
  }
  NFD_LOG_DEBUG("onInterestReject interest=" << pitEntry->getName());

  // cancel unsatisfy & straggler timer
  this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

  // set PIT straggler timer
  this->setStragglerTimer(pitEntry, false);
}

void
Forwarder::onInterestUnsatisfied(const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("onInterestUnsatisfied interest=" << pitEntry->getName());

  // invoke PIT unsatisfied callback
  beforeExpirePendingInterest(*pitEntry);
  this->dispatchToStrategy(*pitEntry,
    [&] (fw::Strategy& strategy) { strategy.beforeExpirePendingInterest(pitEntry); });

  // goto Interest Finalize pipeline
  this->onInterestFinalize(pitEntry, false);
}

void
Forwarder::onInterestFinalize(const shared_ptr<pit::Entry>& pitEntry, bool isSatisfied,
                              time::milliseconds dataFreshnessPeriod)
{
  NFD_LOG_DEBUG("onInterestFinalize interest=" << pitEntry->getName() <<
                (isSatisfied ? " satisfied" : " unsatisfied"));

  // Dead Nonce List insert if necessary
  this->insertDeadNonceList(*pitEntry, isSatisfied, dataFreshnessPeriod, 0);

  // PIT delete
  this->cancelUnsatisfyAndStragglerTimer(*pitEntry);
  m_pit.erase(pitEntry.get());
  // NFD_LOG_DEBUG("onInterestFinalize finish!");
}

void
Forwarder::onIncomingData(Face& inFace, const Data& data)
{
  // std::cout << "OnIncomingData(): " << data.getName() << std::endl;
  // receive Data
  NFD_LOG_DEBUG("onIncomingData face=" << inFace.getId() << " data=" << data.getName());
  data.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
  ++m_counters.nInData;

  // /localhost scope control
  bool isViolatingLocalhost = inFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                              scope_prefix::LOCALHOST.isPrefixOf(data.getName());
  if (isViolatingLocalhost) {
    NFD_LOG_DEBUG("onIncomingData face=" << inFace.getId() <<
                  " data=" << data.getName() << " violates /localhost");
    // (drop)
    return;
  }
  /*
  std::shared_ptr<sd::Entry> sdEntry = m_sd.find(data.getName());
  if (sdEntry != nullptr) {
    std::cout << "receive data from other nodes! Cancel the scheduling data!" << std::endl;
    scheduler::cancel(sdEntry->m_scheduleDataTimer);
    m_sd.erase(sdEntry.get());
    return;
  }
  */
  // do suppression for ack only
  if (data.getName().compare(0, 2, kSyncNotifyPrefix) == 0) {
    auto entry = m_pending_ack.find(data.getName());
    if (entry != m_pending_ack.end()) {
      // std::cout << "node(" << m_id << ") receive same ack from other nodes! Cancel the scheduling ack!" << std::endl;
      m_pending_ack.erase(data.getName());
    }
  }
  // do suppression for sync data
  /*
  if (data.getName().compare(0, 2, kSyncDataPrefix) == 0) {
    auto entry = m_pending_data.find(data.getName());
    if (entry != m_pending_data.end()) {
      m_pending_data.erase(data.getName());
    }
  }
  */

  // PIT match
  pit::DataMatchResult pitMatches = m_pit.findAllDataMatches(data);
  if (pitMatches.begin() == pitMatches.end()) {
    // goto Data unsolicited pipeline
    this->onDataUnsolicited(inFace, data);
    return;
  }

  shared_ptr<Data> dataCopyWithoutTag = make_shared<Data>(data);
  dataCopyWithoutTag->removeTag<lp::HopCountTag>();

  // CS insert
  if (m_csFromNdnSim == nullptr)
    m_cs.insert(*dataCopyWithoutTag);
  else
    m_csFromNdnSim->Add(dataCopyWithoutTag);

  std::set<Face*> pendingDownstreams;
  // foreach PitEntry
  auto now = time::steady_clock::now();
  // std::cout << "Matched pit size: " << pitMatches.size() << std::endl;
  for (const shared_ptr<pit::Entry>& pitEntry : pitMatches) {
    // std::cout << "In-records size: " << pitEntry->getInRecords().size() << std::endl;
    // cancel unsatisfy & straggler timer
    this->cancelUnsatisfyAndStragglerTimer(*pitEntry);

    // remember pending downstreams
    for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
      if (inRecord.getExpiry() > now) {
        pendingDownstreams.insert(&inRecord.getFace());
      }
    }

    // invoke PIT satisfy callback
    beforeSatisfyInterest(*pitEntry, inFace, data);
    this->dispatchToStrategy(*pitEntry,
      [&] (fw::Strategy& strategy) { strategy.beforeSatisfyInterest(pitEntry, inFace, data); });

    // Dead Nonce List insert if necessary (for out-record of inFace)
    this->insertDeadNonceList(*pitEntry, true, data.getFreshnessPeriod(), &inFace);

    // mark PIT satisfied
    /**
     * For data arriving from wifi face, don't clear in-record, so that app
     *  can decide to broadcast
     */
    if (inFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) 
      pitEntry->clearInRecords();
    pitEntry->deleteOutRecord(inFace);
    // std::cout << "Pit In-record Not Removed, size = " << pitEntry->getInRecords().size() << std::endl;

    // set PIT straggler timer
    this->setStragglerTimer(pitEntry, true, data.getFreshnessPeriod());
  }


  // foreach pending downstream
  // std::cout << "Pending downstreams size: " << pendingDownstreams.size();
  for (Face* pendingDownstream : pendingDownstreams) {
    // // if is vsyncData which means in ad hoc networks, we need to send out the data even if outFace == inFace
    // if (data.getName().compare(0, 2, kSyncDataPrefix) == 0) {
    //   this->onOutgoingData(data, *pendingDownstream);
    //   continue;
    // }
    
    if (pendingDownstream == &inFace) {
      continue;
    }
    // goto outgoing Data pipeline
    this->onOutgoingData(data, *pendingDownstream);
  }
}

void
Forwarder::onDataUnsolicited(Face& inFace, const Data& data)
{
  // accept to cache?
  fw::UnsolicitedDataDecision decision = m_unsolicitedDataPolicy->decide(inFace, data);
  if (decision == fw::UnsolicitedDataDecision::CACHE) {
    // CS insert
    if (m_csFromNdnSim == nullptr)
      m_cs.insert(data, true);
    else
      m_csFromNdnSim->Add(data.shared_from_this());
  }

  NFD_LOG_DEBUG("onDataUnsolicited face=" << inFace.getId() <<
                " data=" << data.getName() <<
                " decision=" << decision);
}

void
Forwarder::onOutgoingData(const Data& data, Face& outFace)
{
  // if (data.getName().compare(0, 2, kSyncDataPrefix) == 0 ||
  //   data.getName().compare(0, 2, kSyncNotifyPrefix) == 0) {
  //   // go to another pipeline
  //   if (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL) {
  //     onOutgoingVsyncData(data, outFace);
  //     return;
  //   }
  // }
  if (outFace.getId() == face::INVALID_FACEID) {
    NFD_LOG_WARN("onOutgoingData face=invalid data=" << data.getName());
    return;
  }
  NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() << " data=" << data.getName());

  // /localhost scope control
  bool isViolatingLocalhost = outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                              scope_prefix::LOCALHOST.isPrefixOf(data.getName());
  if (isViolatingLocalhost) {
    NFD_LOG_DEBUG("onOutgoingData face=" << outFace.getId() <<
                  " data=" << data.getName() << " violates /localhost");
    // (drop)
    return;
  }

  // TODO traffic manager
  if (outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL) {
    // std::cout << "Traffic Manager triggered: ";
    if (data.getName().compare(0, 2, kBundledDataPrefix) == 0) {
      m_outBundledData++;
      // std::cout << "Bundled data" << std::endl;
    } else if (data.getName().compare(0, 2, kSyncDataPrefix) == 0) {
      m_outData++;
      // std::cout << "Data" << std::endl;
    } else if (data.getName().compare(0, 2, kSyncNotifyPrefix) == 0) {
      m_outAck++;
      // std::cout << "ACK" << std::endl;
    }
  }

  // simulate packet loss at the sender side
  if ((data.getName().compare(0, 2, kSyncDataPrefix) == 0 ||
       data.getName().compare(0, 2, kBundledDataPrefix) == 0) &&
      outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
      m_loss_rate != 0.0)
  { 
    uint64_t number = getRandomNumber(0, 100);
    double bound = m_loss_rate * 100;
    if (number >= 0 && number < bound) {
      return;
    }
  }

  // send Data
  outFace.sendData(data);
  ++m_counters.nOutData;
}

void Forwarder::onOutgoingVsyncData(const Data& data, Face& outFace) {
  const Name& n = data.getName();
  if (m_pending_data.find(n) != m_pending_data.end()) return;
  if (m_pending_ack.find(n) != m_pending_ack.end()) return;
  if (data.getName().compare(0, 2, kSyncDataPrefix) == 0) {
    m_pending_data[n] = std::pair<std::shared_ptr<const Data>, std::shared_ptr<Face>>(data.shared_from_this(), outFace.shared_from_this()); 
  }
  else m_pending_ack[n] = std::pair<std::shared_ptr<const Data>, std::shared_ptr<Face>>(data.shared_from_this(), outFace.shared_from_this());
  if (in_data_dt) return;
  else {
    in_data_dt = true;
    uint64_t t_dataout = getRandomNumber(0, kDataOut);
    data_dt = scheduler::schedule(time::microseconds(t_dataout), bind(&Forwarder::onVsyncDataDTTimeout, this));
  }
  /*
  shared_ptr<sd::Entry> sdEntry = m_sd.insert(data);
  BOOST_ASSERT(sdEntry != nullptr);
  uint64_t t_dataout = getRandomNumber(0, kDataOut);
  sdEntry->m_scheduleDataTimer = scheduler::schedule(time::milliseconds(t_dataout), 
    bind(&Forwarder::onVsyncDataSchedulerTimeout, this, cref(data), ref(outFace), sdEntry));
  */
  /*
  shared_ptr<sd::Entry> sdEntry = m_sd.insert(data);
  BOOST_ASSERT(sdEntry != nullptr);
  uint64_t t_dataout = getRandomNumber(0, kDataOut);
  vsyncDataScheduler = scheduler::schedule(time::milliseconds(t_dataout),
    bind(&Forwarder::onVsyncDataSchedulerTimeout, this, cref(data), ref(outFace), sdEntry));
    */
}

void Forwarder::onVsyncDataDTTimeout() {
  // the pending_ack has higher priority than pending_data
  NFD_LOG_DEBUG("onVsyncDataDTTimeout");
  if (m_pending_data.empty() && m_pending_ack.empty()) {
    in_data_dt = false;
    return;
  }

  std::shared_ptr<Face> outFace;
  std::shared_ptr<const Data> outData;
  Name n;
  bool sending_ack = false;
  if (!m_pending_ack.empty()) {
    outFace = m_pending_ack.begin()->second.second;
    outData = m_pending_ack.begin()->second.first;
    n = m_pending_ack.begin()->first;
    sending_ack = true;
    m_outAck++;
  }
  else {
    outFace = m_pending_data.begin()->second.second;
    outData = m_pending_data.begin()->second.first;
    n = m_pending_data.begin()->first;
    m_outData++;
  }

  // std::cout << "node(" << m_id << ") data DT time out, the current pending data list size = " << m_pending_data.size() << ", now sending data name = " << n.toUri() << "to face = " << outFace->getId() << std::endl;
  if (outFace->getId() == face::INVALID_FACEID) {
    NFD_LOG_WARN("onOutgoingData face=invalid data=" << n);
    return;
  }

  // /localhost scope control
  bool isViolatingLocalhost = outFace->getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL &&
                              scope_prefix::LOCALHOST.isPrefixOf(n);
  if (isViolatingLocalhost) {
    NFD_LOG_DEBUG("onOutgoingData face=" << outFace->getId() <<
                  " data=" << n << " violates /localhost");
    // (drop)
    return;
  }

  // simulate packet loss at the sender side
  bool packet_loss = false;
  if (outFace->getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL && m_loss_rate != 0.0)
  { 
    uint64_t number = getRandomNumber(0, 100);
    double bound = m_loss_rate * 100;
    if (number >= 0 && number < bound) {
      packet_loss = true;
    }
  }
  if (!packet_loss) {
    NFD_LOG_DEBUG("onOutgoingVsyncData face=" << outFace->getId() << " data=" << n);
    outFace->sendData(*outData);
  }

  if (sending_ack) {
    if (m_pending_ack.find(n) != m_pending_ack.end()) m_pending_ack.erase(n);
  }
  else m_pending_data.erase(n);

  if (m_pending_data.empty() && m_pending_ack.empty()) {
    in_data_dt = false;
    return;
  }
  uint64_t t_dataout = getRandomNumber(0, kDataOut);
  data_dt = scheduler::schedule(time::microseconds(t_dataout), bind(&Forwarder::onVsyncDataDTTimeout, this));
}

void
Forwarder::onIncomingNack(Face& inFace, const lp::Nack& nack)
{
  // receive Nack
  nack.setTag(make_shared<lp::IncomingFaceIdTag>(inFace.getId()));
  ++m_counters.nInNacks;

  // if multi-access face, drop
  if (inFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) {
    NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                  " nack=" << nack.getInterest().getName() <<
                  "~" << nack.getReason() << " face-is-multi-access");
    return;
  }

  // PIT match
  shared_ptr<pit::Entry> pitEntry = m_pit.find(nack.getInterest());
  // if no PIT entry found, drop
  if (pitEntry == nullptr) {
    NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                  " nack=" << nack.getInterest().getName() <<
                  "~" << nack.getReason() << " no-PIT-entry");
    return;
  }

  // has out-record?
  pit::OutRecordCollection::iterator outRecord = pitEntry->getOutRecord(inFace);
  // if no out-record found, drop
  if (outRecord == pitEntry->out_end()) {
    NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                  " nack=" << nack.getInterest().getName() <<
                  "~" << nack.getReason() << " no-out-record");
    return;
  }

  // if out-record has different Nonce, drop
  if (nack.getInterest().getNonce() != outRecord->getLastNonce()) {
    NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                  " nack=" << nack.getInterest().getName() <<
                  "~" << nack.getReason() << " wrong-Nonce " <<
                  nack.getInterest().getNonce() << "!=" << outRecord->getLastNonce());
    return;
  }

  NFD_LOG_DEBUG("onIncomingNack face=" << inFace.getId() <<
                " nack=" << nack.getInterest().getName() <<
                "~" << nack.getReason() << " OK");

  // record Nack on out-record
  outRecord->setIncomingNack(nack);

  // trigger strategy: after receive NACK
  this->dispatchToStrategy(*pitEntry,
    [&] (fw::Strategy& strategy) { strategy.afterReceiveNack(inFace, nack, pitEntry); });
}

void
Forwarder::onOutgoingNack(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
                          const lp::NackHeader& nack)
{
  if (outFace.getId() == face::INVALID_FACEID) {
    NFD_LOG_WARN("onOutgoingNack face=invalid" <<
                  " nack=" << pitEntry->getInterest().getName() <<
                  "~" << nack.getReason() << " no-in-record");
    return;
  }

  // has in-record?
  pit::InRecordCollection::iterator inRecord = pitEntry->getInRecord(outFace);

  // if no in-record found, drop
  if (inRecord == pitEntry->in_end()) {
    NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                  " nack=" << pitEntry->getInterest().getName() <<
                  "~" << nack.getReason() << " no-in-record");
    return;
  }

  // if multi-access face, drop
  if (outFace.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) {
    NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                  " nack=" << pitEntry->getInterest().getName() <<
                  "~" << nack.getReason() << " face-is-multi-access");
    return;
  }

  NFD_LOG_DEBUG("onOutgoingNack face=" << outFace.getId() <<
                " nack=" << pitEntry->getInterest().getName() <<
                "~" << nack.getReason() << " OK");

  // create Nack packet with the Interest from in-record
  lp::Nack nackPkt(inRecord->getInterest());
  nackPkt.setHeader(nack);

  // erase in-record
  pitEntry->deleteInRecord(outFace);

  // send Nack on face
  const_cast<Face&>(outFace).sendNack(nackPkt);
  ++m_counters.nOutNacks;
}

static inline bool
compare_InRecord_expiry(const pit::InRecord& a, const pit::InRecord& b)
{
  return a.getExpiry() < b.getExpiry();
}

void
Forwarder::setUnsatisfyTimer(const shared_ptr<pit::Entry>& pitEntry)
{
  pit::InRecordCollection::iterator lastExpiring =
    std::max_element(pitEntry->in_begin(), pitEntry->in_end(), &compare_InRecord_expiry);

  time::steady_clock::TimePoint lastExpiry = lastExpiring->getExpiry();
  time::nanoseconds lastExpiryFromNow = lastExpiry - time::steady_clock::now();
  if (lastExpiryFromNow <= time::seconds::zero()) {
    // TODO all in-records are already expired; will this happen?
  }

  scheduler::cancel(pitEntry->m_unsatisfyTimer);
  pitEntry->m_unsatisfyTimer = scheduler::schedule(lastExpiryFromNow,
    bind(&Forwarder::onInterestUnsatisfied, this, pitEntry));
}

void
Forwarder::setStragglerTimer(const shared_ptr<pit::Entry>& pitEntry, bool isSatisfied,
                             time::milliseconds dataFreshnessPeriod)
{
  time::nanoseconds stragglerTime = time::milliseconds(100);

  scheduler::cancel(pitEntry->m_stragglerTimer);
  pitEntry->m_stragglerTimer = scheduler::schedule(stragglerTime,
    bind(&Forwarder::onInterestFinalize, this, pitEntry, isSatisfied, dataFreshnessPeriod));
}

void
Forwarder::cancelUnsatisfyAndStragglerTimer(pit::Entry& pitEntry)
{
  scheduler::cancel(pitEntry.m_unsatisfyTimer);
  scheduler::cancel(pitEntry.m_stragglerTimer);
}

static inline void
insertNonceToDnl(DeadNonceList& dnl, const pit::Entry& pitEntry,
                 const pit::OutRecord& outRecord)
{
  dnl.add(pitEntry.getName(), outRecord.getLastNonce());
}

void
Forwarder::insertDeadNonceList(pit::Entry& pitEntry, bool isSatisfied,
                               time::milliseconds dataFreshnessPeriod, Face* upstream)
{
  // need Dead Nonce List insert?
  bool needDnl = false;
  if (isSatisfied) {
    bool hasFreshnessPeriod = dataFreshnessPeriod >= time::milliseconds::zero();
    // Data never becomes stale if it doesn't have FreshnessPeriod field
    needDnl = static_cast<bool>(pitEntry.getInterest().getMustBeFresh()) &&
              (hasFreshnessPeriod && dataFreshnessPeriod < m_deadNonceList.getLifetime());
  }
  else {
    needDnl = true;
  }

  if (!needDnl) {
    return;
  }

  // Dead Nonce List insert
  if (upstream == 0) {
    // insert all outgoing Nonces
    const pit::OutRecordCollection& outRecords = pitEntry.getOutRecords();
    std::for_each(outRecords.begin(), outRecords.end(),
                  bind(&insertNonceToDnl, ref(m_deadNonceList), cref(pitEntry), _1));
  }
  else {
    // insert outgoing Nonce of a specific face
    pit::OutRecordCollection::iterator outRecord = pitEntry.getOutRecord(*upstream);
    if (outRecord != pitEntry.getOutRecords().end()) {
      m_deadNonceList.add(pitEntry.getName(), outRecord->getLastNonce());
    }
  }
}

} // namespace nfd
