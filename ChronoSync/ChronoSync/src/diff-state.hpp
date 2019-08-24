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
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_DIFF_STATE_HPP
#define CHRONOSYNC_DIFF_STATE_HPP

#include "state.hpp"

namespace chronosync {

class DiffState;
using DiffStatePtr = shared_ptr<DiffState>;
using ConstDiffStatePtr = shared_ptr<const DiffState>;

/**
 * @brief Contains the diff info between two states.
 *
 * DiffState is used to construct DiffLog.  It serves as
 * a log entry.  Each log entry contains the updates between
 * two states, and is indexed by the digest of the second state
 * which is the result when the updates have been applied.
 *
 * DiffLog is a chain of DiffStates. Each DiffState connects to
 * the next DiffState (a newer diff) through member m_next. The
 * m_next of the last DiffState in a log should be empty. And the
 * root digest of the last DiffState in the log should be the most
 * current state.
 */
class DiffState : public State
{
public:
  /**
   * @brief Set successor for the diff state
   *
   * @param next successor state
   */
  void
  setNext(ConstDiffStatePtr next)
  {
    m_next = next;
  }

  /**
   * @brief Set digest for the diff state (obtained from a corresponding full state)
   *
   * @param digest root digest of the full state
   */
  void
  setRootDigest(ConstBufferPtr digest)
  {
    m_digest = digest;
  }

  /**
   * @brief Get root digest of the full state after applying the diff state
   */
  ConstBufferPtr
  getRootDigest() const
  {
    return m_digest;
  }

  /**
   * @brief Accumulate differences from this state to the most current state
   *
   * This method assumes that the DiffState is in a log. It will iterate the all
   * the DiffState between its m_next DiffState and the last DiffState in the log,
   * and aggregate all the differences into one diff, which is represented as a
   * State object.
   *
   * @returns Accumulated differences from this state to the most current state
   */
  ConstStatePtr
  diff() const;

private:
  ConstDiffStatePtr m_next;
  ConstBufferPtr m_digest;
};

} // namespace chronosync

#endif // CHRONOSYNC_DIFF_STATE_HPP
