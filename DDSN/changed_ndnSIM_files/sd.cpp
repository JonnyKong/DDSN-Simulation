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

#include "sd.hpp"

namespace nfd {
namespace sd {

shared_ptr<Entry>
Sd::find(const Data& data) const
{
  auto it = std::find_if(m_sdEntries.begin(), m_sdEntries.end(),
      [&data] (const shared_ptr<Entry>& entry) {
        return entry->canMatch(data);
      });
  if (it == m_sdEntries.end()) return nullptr;
  return *it;
}

shared_ptr<Entry>
Sd::insert(const Data& data)
{
  /*
  auto it = std::find_if(m_sdEntries.begin(), m_sdEntries.end(),
      [&data] (const shared_ptr<Entry>& entry) {
        return entry->canMatch(data);
      });
  //BOOST_ASSERT(it == m_sdEntries.end());
  if (it != m_sdEntries.end()) return *it;
  */
  auto entry = make_shared<Entry>(data);
  m_sdEntries.push_back(entry);
  ++m_nItems;
  return entry;
}

void 
Sd::erase(Entry* entry) 
{
  BOOST_ASSERT(entry != nullptr);
  auto it = std::find_if(m_sdEntries.begin(), m_sdEntries.end(),
    [entry] (const shared_ptr<Entry>& sitEntry2) { return sitEntry2.get() == entry; });
  BOOST_ASSERT(it != m_sdEntries.end());

  *it = m_sdEntries.back();
  m_sdEntries.pop_back();
  --m_nItems;
}

void
Sd::clear() {
  for (auto entry: m_sdEntries) {
    scheduler::cancel(entry->m_scheduleDataTimer);
  }
  m_sdEntries.clear();
}

} // namespace sd
} // namespace nfd