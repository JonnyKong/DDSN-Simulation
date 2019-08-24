/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2017 University of California, Los Angeles
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
 */

#include "leaf.hpp"

namespace chronosync {

Leaf::Leaf(const Name& sessionName, const SeqNo& seq)
  : m_sessionName(sessionName)
  , m_seq(seq)
{
  updateDigest();
}

Leaf::Leaf(const Name& userPrefix, uint64_t session, const SeqNo& seq)
  : m_sessionName(userPrefix)
  , m_seq(seq)
{
  m_sessionName.appendNumber(session);
  updateDigest();
}

Leaf::~Leaf()
{
}

ConstBufferPtr
Leaf::getDigest() const
{
  return m_digest.computeDigest();
}

void
Leaf::setSeq(const SeqNo& seq)
{
  if (seq > m_seq) {
    m_seq = seq;
    updateDigest();
  }
}

void
Leaf::updateDigest()
{
  m_digest.reset();
  m_digest << getSessionName().wireEncode() << getSeq();
  m_digest.computeDigest();
}

std::ostream&
operator<<(std::ostream& os, const Leaf& leaf)
{
  os << leaf.getSessionName() << "(" << leaf.getSeq() << ")";
  return os;
}


} // namespace chronosync
