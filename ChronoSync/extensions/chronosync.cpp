/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 */

#include "chronosync.hpp"

namespace ndn {

ChronoSync::ChronoSync(uint64_t nid, const int minNumberMessages, const int maxNumberMessages)
  : m_nid(nid)
  , m_face(m_ioService)
  , m_scheduler(m_ioService)
  , m_randomGenerator(nid)
  // , m_rangeUniformRandom(m_randomGenerator, boost::uniform_int<>(1000, 3000))
  , m_rangeUniformRandom(m_randomGenerator, boost::uniform_int<>(40000 * 0.9, 40000 * 1.1))
  // , m_rangeUniformRandom(m_randomGenerator, boost::uniform_int<>(10000 * 0.9, 10000 * 1.1))
  , m_forwardUniformRandom(m_randomGenerator, boost::uniform_int<>(0, 100))
  , m_numberMessages(1)
{
  // Print NFD traffic at the end of simulation
  m_scheduler.scheduleEvent(time::seconds(2395), [this] {
    printNFDTraffic();
  });
}

void
ChronoSync::setSyncPrefix(const Name& syncPrefix)
{
  m_syncPrefix = syncPrefix;
}

void
ChronoSync::setUserPrefix(const Name& userPrefix)
{
  m_userPrefix = userPrefix;
}

void
ChronoSync::setRoutingPrefix(const Name& routingPrefix)
{
  m_routingPrefix = routingPrefix;
}

void
ChronoSync::setDataGenerationDuration(const int dataGenerationDuration)
{
  m_dataGenerationDuration = dataGenerationDuration;  
}

void
ChronoSync::delayedInterest(int id)
{
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  if (now > (int64_t)m_dataGenerationDuration * 1000 * 1000) { 
    return;
  }
  std::cout << now << " microseconds node(" << m_nid 
            << ") Publish data with id: " << id << "\n";
  m_socket->publishData(reinterpret_cast<const uint8_t*>(std::to_string(id).c_str()),
                        std::to_string(id).size(), ndn::time::milliseconds(4000));


  // if (id < m_numberMessages)
  //   m_scheduler.scheduleEvent(ndn::time::milliseconds(m_rangeUniformRandom()),
  //                             bind(&ChronoSync::delayedInterest, this, ++id));
}

void
ChronoSync::publishDataPeriodically(int id)
{ 
  // m_scheduler.scheduleEvent(ndn::time::milliseconds(m_rangeUniformRandom()),
  //                           bind(&ChronoSync::delayedInterest, this, id));
  delayedInterest(id);

  m_scheduler.scheduleEvent(ndn::time::milliseconds(m_rangeUniformRandom()),
                            bind(&ChronoSync::publishDataPeriodically, this, id + 1));
}


void
ChronoSync::printData(const Data& data)
{
  Name::Component peerName = data.getName().at(1);

  std::string s (reinterpret_cast<const char*>(data.getContent().value()),
                 data.getContent().value_size());
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  // std::cout << now << " microseconds node(" << m_nid 
  //           << ") Data received from " << peerName.toUri() << " : " <<  s << "\n";
  auto data_name = data.getName().getSubName(1);

  std::lock_guard<std::mutex> guard(m_dataFetchedMutex);
  if (m_dataFetched.find(data_name) == m_dataFetched.end()) {
    m_dataFetched.insert(data_name);
    std::cout << now << " microseconds node(" << m_nid 
              << ") Store New Data: " << data_name.toUri() << "\n";
  }
}

void
ChronoSync::noDataCallback(const Interest& interest)
{
  // This data interest can't be satisfied, forward again with 50% probability.
  // Forwarded data interest doesn't get retransmitted.
  int prob = m_forwardUniformRandom();
  if (prob > 50)
    return;
  
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid 
            << ") noDataCallback(): " << interest.getName() << "\n";

  Name sessionName = interest.getName().getSubName(-3, 2);
  std::string lastComponent = interest.getName().getSubName(-1, 1).toUri().substr(1);
  lastComponent.erase(std::remove(lastComponent.begin(), lastComponent.end(), '%'), lastComponent.end());
  chronosync::SeqNo seq = std::stoul(lastComponent, nullptr, 16);
  std::cout << "Forward data: " << sessionName << "/" << seq << std::endl;
  m_socket->fetchData(sessionName, seq,
                      bind(&ChronoSync::printData, this, _1),
                      0, true);
}

void
ChronoSync::processSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  // int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  // std::cout << now << " microseconds node(" << m_nid << ") Process Sync Update \n";
  if (updates.empty()) {
    return;
  }

  for (unsigned int i = 0; i < updates.size(); i++) {
    for (chronosync::SeqNo seq = updates[i].low; seq <= updates[i].high; ++seq) {
      m_socket->fetchData(updates[i].session, seq,
                          bind(&ChronoSync::printData, this, _1),
                          9);
    }

  }
}

void
ChronoSync::initializeSync()
{
  std::cout << "ChronoSync Instance Initialized \n";
  m_socket = std::make_shared<chronosync::Socket>(m_syncPrefix,
                                                  m_routingPrefix,
                                                  m_userPrefix,
                                                  m_face,
                                                  bind(&ChronoSync::processSyncUpdate, this, _1));
  m_socket->setNodeID(m_nid);
  m_socket->setKeepDataCopy();
  m_socket->setNoDataCallback(std::bind(&ChronoSync::noDataCallback, this, _1));
}

void
ChronoSync::run()
{
  m_scheduler.scheduleEvent(ndn::time::milliseconds(m_rangeUniformRandom()),
                            bind(&ChronoSync::delayedInterest, this, 1));
}

void
ChronoSync::runPeriodically()
{
  m_scheduler.scheduleEvent(ndn::time::milliseconds(100 * m_nid),
                            bind(&ChronoSync::publishDataPeriodically, this, 1));
}

void
ChronoSync::printNFDTraffic()
{
  Interest i("/ndn/getNDNTraffic", time::milliseconds(5));
  m_face.expressInterest(i, [](const Interest&, const Data&) {},
                         [](const Interest&, const lp::Nack&) {},
                         [](const Interest&) {});
}


} // namespace ndn
