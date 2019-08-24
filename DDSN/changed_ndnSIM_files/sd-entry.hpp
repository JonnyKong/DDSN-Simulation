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

#ifndef NFD_DAEMON_TABLE_SD_ENTRY_HPP
#define NFD_DAEMON_TABLE_SD_ENTRY_HPP

#include "face/face.hpp"
#include "core/common.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace sd {

class Entry : public noncopyable
{
public:
  explicit
  Entry(const Data& data): m_data(data.shared_from_this()) {
  }

  /** \return the representative data of the SD entry
   */
  const Data&
  getData() const
  {
    return *m_data;
  }

  /** \return Data Name
   */
  const Name&
  getName() const
  {
    return m_data->getName();
  }

  bool
  canMatch(const Data& data) const 
  {
    return m_data->getName().compare(0, Name::npos,
                                         data.getName(), 0, Name::npos) == 0;
  }

  void 
  setTimeOut() 
  {
    timeout = true;
  }

  bool
  isTimeOut() const
  {
    return timeout;
  }


public:
  /** \brief scheduled data timer
   */
  scheduler::EventId m_scheduleDataTimer;

private:
  shared_ptr<const Data> m_data;
  bool timeout = false;
};

} // namespace pit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_ENTRY_HPP