    /* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2018 University of California, Los Angeles
 *
 * This file is part of ChronoSync, synchronization library for distributed realtime
 * applications for NDN.
 *
 * ChronoSync is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * ChronoSync is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ChronoSync, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 * @author Chaoyi Bian <bcy@pku.edu.cn>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "ns3/nstime.h"
#include <ndn-cxx/face.hpp>

#include "socket.hpp"
#include "logger.hpp"
#include <time.h>

INIT_LOGGER(Socket);
// NS_LOG_COMPONENT_DEFINE("ChronoSocket");

namespace chronosync {

const ndn::Name Socket::DEFAULT_NAME;
const ndn::Name Socket::DEFAULT_PREFIX;
const std::shared_ptr<Validator> Socket::DEFAULT_VALIDATOR;

Socket::Socket(const Name& syncPrefix,
               const Name& routingPrefix,
               const Name& userPrefix,
               ndn::Face& face,
               const UpdateCallback& updateCallback,
               const Name& signingId,
               std::shared_ptr<Validator> validator,
               const time::milliseconds& syncInterestLifetime,
               const name::Component& session)
  : m_routingPrefix(routingPrefix)
  , m_userPrefix(Name("").append(routingPrefix).append(userPrefix))
  , m_face(face)
  , m_logic(face, syncPrefix, userPrefix, updateCallback, Logic::DEFAULT_NAME, Logic::DEFAULT_VALIDATOR,
            Logic::DEFAULT_RESET_TIMER, Logic::DEFAULT_CANCEL_RESET_TIMER, Logic::DEFAULT_RESET_INTEREST_LIFETIME,
            syncInterestLifetime, Logic::DEFAULT_SYNC_REPLY_FRESHNESS, Logic::DEFAULT_RECOVERY_INTEREST_LIFETIME,
            session)
  , m_signingId(signingId)
  , m_keyChain(ns3::ndn::StackHelper::getKeyChain())
  , m_validator(validator)
  , m_scheduler(face.getIoService())
  , m_keep_data_copy(false)
  , m_random_generator(std::time(0))
  , m_data_interest_random(m_random_generator, boost::uniform_int<>(0, 1000))
{
  NDN_LOG_DEBUG(">> Socket::Socket");
  std::cout << "m_userPrefix: " << m_userPrefix << std::endl;
  if (m_userPrefix != DEFAULT_NAME)
    m_registeredPrefixList[m_userPrefix] =
      m_face.setInterestFilter(m_routingPrefix,
                               bind(&Socket::onInterest, this, _1, _2),
                               [] (const Name& prefix, const std::string& msg) {});
  NDN_LOG_DEBUG("<< Socket::Socket");
}

Socket::~Socket()
{
  for(const auto& itr : m_registeredPrefixList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }
  m_ims.erase("/");
}

void
Socket::addSyncNode(const Name& prefix, const Name& signingId, const name::Component& session)
{
  if (prefix == DEFAULT_NAME)
    return;

  NDN_LOG_DEBUG(">> addSyncNode");

  auto itr = m_registeredPrefixList.find(prefix);
  if (itr != m_registeredPrefixList.end())
    return;

  if (m_userPrefix == DEFAULT_NAME)
    m_userPrefix = prefix;
  m_logic.addUserNode(prefix, signingId, session);
  m_registeredPrefixList[prefix] =
    m_face.setInterestFilter(prefix,
                             bind(&Socket::onInterest, this, _1, _2),
                             [] (const Name& prefix, const std::string& msg) {});
  NDN_LOG_DEBUG("<< addSyncNode");
}

void
Socket::removeSyncNode(const Name& prefix)
{
  if (prefix == DEFAULT_NAME)
    return;

  auto itr = m_registeredPrefixList.find(prefix);
  if (itr != m_registeredPrefixList.end()) {
    if (static_cast<bool>(itr->second))
      m_face.unsetInterestFilter(itr->second);
    m_registeredPrefixList.erase(itr);
  }

  m_ims.erase(prefix);
  m_logic.removeUserNode(prefix);
}

void
Socket::publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
                    const Name& prefix)
{
  publishData(ndn::encoding::makeBinaryBlock(ndn::tlv::Content, buf, len), freshness, prefix);
}

void
Socket::publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
                    const uint64_t& seqNo, const Name& prefix)
{
  publishData(ndn::encoding::makeBinaryBlock(ndn::tlv::Content, buf, len), freshness, seqNo, prefix);
}

void
Socket::publishData(const Block& content, const ndn::time::milliseconds& freshness,
                    const Name& prefix)
{
  shared_ptr<Data> data = make_shared<Data>();
  data->setContent(content);
  data->setFreshnessPeriod(freshness);

  SeqNo newSeq = m_logic.getSeqNo(prefix) + 1;
  Name dataName("/chronosync");
  dataName.append(m_logic.getSessionName(prefix)).appendNumber(newSeq);
  data->setName(dataName);
  
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Update New Seq: "
            << dataName << std::endl;
  std::cout << now << " microseconds node(" << m_nid << ") Store New Data: "
            << dataName.getSubName(1) << std::endl;

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_ims.insert(*data);

  m_logic.updateSeqNo(newSeq, prefix);
}

void
Socket::publishData(const Block& content, const ndn::time::milliseconds& freshness,
                    const uint64_t& seqNo, const Name& prefix)
{  
  shared_ptr<Data> data = make_shared<Data>();
  data->setContent(content);
  data->setFreshnessPeriod(freshness);

  SeqNo newSeq = seqNo;
  Name dataName("/chronosync");
  dataName.append(m_logic.getSessionName(prefix)).appendNumber(newSeq);
  data->setName(dataName);
 
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Update New Seq: "
            << dataName << std::endl;
  std::cout << now << " microseconds node(" << m_nid << ") Store New Data: "
            << dataName.getSubName(1) << std::endl;
 
  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_ims.insert(*data);

  m_logic.updateSeqNo(newSeq, prefix);
}

void
Socket::fetchData(const Name& sessionName, const SeqNo& seqNo,
                  const DataValidatedCallback& dataCallback,
                  int nRetries, bool isFwd)
{
  Name interestName;
  interestName.append(m_routingPrefix).append(sessionName).appendNumber(seqNo);

  Interest interest(interestName);
  // interest.setMustBeFresh(true);
  interest.setInterestLifetime(time::seconds(5));

  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  if (!isFwd) {
    std::cout << now << " microseconds node(" << m_nid << ") Update New Seq: "
              << interestName << std::endl;
  }

  DataValidationErrorCallback failureCallback =
    bind(&Socket::onDataValidationFailed, this, _1, _2);
  
  // Add random timer for collision avoidance
  int delay = m_data_interest_random();
  m_scheduler.scheduleEvent(time::milliseconds(delay), 
                            [this, interest, interestName, dataCallback, failureCallback, nRetries, isFwd] {
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << now << " microseconds node(" << m_nid << ") Send Data Interest (" 
              << (isFwd ? "Forward" : "1") << "): " << interestName << std::endl;
    m_face.expressInterest(interest,
                           bind(&Socket::onData, this, _1, _2, dataCallback, failureCallback),
                           bind(&Socket::onDataTimeout, this, _1, nRetries,
                                dataCallback, failureCallback, now, "NACK"), // Nack
                           bind(&Socket::onDataTimeout, this, _1, nRetries,
                                dataCallback, failureCallback, now, "TIMEOUT"));
  });
}

void
Socket::fetchData(const Name& sessionName, const SeqNo& seqNo,
                  const DataValidatedCallback& dataCallback,
                  const DataValidationErrorCallback& failureCallback,
                  const ndn::TimeoutCallback& onTimeout,
                  int nRetries)
{
  _LOG_DEBUG(">>> Fetching data> Socket::fetchData");
  Name interestName;
  interestName.append(sessionName).appendNumber(seqNo);

  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Update New Seq: "
            << interestName << std::endl;

  Interest interest(interestName);
  // interest.setMustBeFresh(true);
  
  // Add random timer for collision avoidance
  int delay = m_data_interest_random();
  m_scheduler.scheduleEvent(time::milliseconds(delay), 
                            [this, interest, interestName, dataCallback, failureCallback, onTimeout] {
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << now << " microseconds node(" << m_nid << ") Send Data Interest (2): "
            << interestName << std::endl;
    m_face.expressInterest(interest,
                           bind(&Socket::onData, this, _1, _2, dataCallback, failureCallback),
                           bind(onTimeout, _1), // Nack
                           onTimeout);
  });

  _LOG_DEBUG("<< Socket::fetchData");
}

void
Socket::onInterest(const Name& prefix, const Interest& interest)
{
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now <<  " microseconds node(" << m_nid << ") Received data interest: "
            << interest.getName().toUri() << std::endl;
  
  shared_ptr<const Data> data = m_ims.find(interest);
  
  // Return if data in data store
  if (data != nullptr) {
    // Add random timer for collision avoidance
    int delay = m_data_interest_random();
    m_scheduler.scheduleEvent(time::milliseconds(delay), [this, interest, data]{
      int64_t now = ns3::Simulator::Now().GetMicroSeconds();
      std::cout << now << " microseconds node(" << m_nid << ") Send Data Reply "
                << interest.getName().toUri() << std::endl;
      m_face.put(*data);
    });
  } else if (m_no_data_callback) {
    // Notify app that received data interest can't be satisfied, so app can decide
    //  whether to forward it.
    m_no_data_callback(interest);
  }
}

void
Socket::onData(const Interest& interest, const Data& data,
               const DataValidatedCallback& onValidated,
               const DataValidationErrorCallback& onFailed)
{
  _LOG_DEBUG("Socket::onData");

  if (static_cast<bool>(m_validator)) {
    m_validator->validate(data, onValidated, onFailed);
  } else {
    if (m_keep_data_copy)
      m_ims.insert(data);
    onValidated(data);
  }
}

void
Socket::onDataTimeout(const Interest& interest, int nRetries,
                      const DataValidatedCallback& onValidated,
                      const DataValidationErrorCallback& onFailed,
                      int64_t last, const std::string& reason)
{
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  // std::cout << now << " microseconds node(" << m_nid << ") onDataTimeout()\n";
  _LOG_DEBUG("Socket::onDataTimeout");
  if (nRetries <= 0)
    return;

  Interest newNonceInterest(interest);
  newNonceInterest.refreshNonce();
  newNonceInterest.setInterestLifetime(time::milliseconds(5000));
  // newNonceInterest.setMustBeFresh(true);

  int delay = m_data_interest_random();

  // Should wait for longer time if received NACK
  if (reason == "NACK" && now < last + 5 * 1000 * 1000)
    delay += 5 * 1000 - (now - last) / 1000;

  m_scheduler.scheduleEvent(time::milliseconds(delay), 
                            [this, newNonceInterest, nRetries, reason, onValidated, onFailed] {
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << now << " microseconds node(" << m_nid << ") Send Data Interest (" 
              << reason << "): " << newNonceInterest.getName().toUri() << std::endl;
    m_face.expressInterest(newNonceInterest,
                           bind(&Socket::onData, this, _1, _2, onValidated, onFailed),
                           bind(&Socket::onDataTimeout, this, _1, nRetries - 1,
                                onValidated, onFailed, now, "NACK"), // Nack
                           bind(&Socket::onDataTimeout, this, _1, nRetries - 1,
                                onValidated, onFailed, now, "TIMEOUT"));
  });
}

void
Socket::onDataValidationFailed(const Data& data,
                               const ValidationError& error)
{
}

ConstBufferPtr
Socket::getRootDigest() const
{
  return m_logic.getRootDigest();
}

} // namespace chronosync
