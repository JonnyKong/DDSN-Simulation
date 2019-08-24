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

#ifndef NFD_DAEMON_TABLE_VST_HPP
#define NFD_DAEMON_TABLE_VST_HPP

#include "vst-entry.hpp"

namespace nfd {
namespace vst {

/** \brief represents the Interest Table
 */
class Vst : noncopyable
{
public:
  Vst()
  {
    m_nItems = 0;
  }

  /** \return number of entries
   */
  size_t
  size() const
  {
    return m_nItems;
  }

  shared_ptr<Entry>
  find(const Interest& interest) const;

  shared_ptr<Entry>
  insert(const Interest& interest);

  /** \brief deletes an entry
   */
  void
  erase(Entry* entry);

private:
  std::vector<shared_ptr<vst::Entry>> m_vstEntries;
  size_t m_nItems;
};

} // namespace vst

using vst::Vst;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_VST_HPP