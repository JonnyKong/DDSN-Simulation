/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */
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
 * Author: Zhaoning Kong <jonnykong@cs.ucla.edu>
 */

#include "logging.hpp"

namespace ndn {
namespace vsync {
Logger::Logger(uint64_t nid)
  : m_nid(nid)
  , is_enabled(true)
{
}

void
Logger::enable()
{
  is_enabled = true;
}

void
Logger::disable()
{
  is_enabled = false;
}

void
Logger::logDataStore(const Name& name)
{
  if (!is_enabled)
    return;
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Store New Data: " << name.toUri() << std::endl;
}

void
Logger::logStateStore(const NodeID& nid, int64_t seq)
{
  if (!is_enabled)
    return;
  std::string state_tag = to_string(nid) + "-" + to_string(seq);
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Update New Seq: "
            << state_tag << std::endl;
}

void
Logger::logDist(double dist)
{
  if (!is_enabled)
    return;
  int64_t now = ns3::Simulator::Now().GetMicroSeconds();
  std::cout << now << " microseconds node(" << m_nid << ") Travelled Distance: "
            << dist << std::endl;
}


} // namespace vsync
} // namespace ndn
