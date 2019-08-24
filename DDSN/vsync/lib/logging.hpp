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

#ifndef NDN_VSYNC_LOGGING_HPP_
#define NDN_VSYNC_LOGGING_HPP_

/*
#define NS3_LOG_ENABLE
#ifdef NS3_LOG_ENABLE
*/

#include "ns3/log.h"
#include "ndn-common.hpp"
#include "vsync-common.hpp"

#define VSYNC_LOG_DEFINE(name) NS_LOG_COMPONENT_DEFINE(#name)

#define VSYNC_LOG_TRACE(expr) NS_LOG_LOGIC(expr)
#define VSYNC_LOG_INFO(expr) NS_LOG_INFO(expr)
#define VSYNC_LOG_DEBUG(expr) NS_LOG_DEBUG(expr)
#define VSYNC_LOG_WARN(expr) NS_LOG_WARN(expr)
#define VSYNC_LOG_ERROR(expr) NS_LOG_ERROR(expr)

/*
#else

#include <ndn-cxx/util/logger.hpp>

#define VSYNC_LOG_DEFINE(name) NDN_LOG_INIT(name)

#define VSYNC_LOG_TRACE(expr) NDN_LOG_TRACE(expr)
#define VSYNC_LOG_INFO(expr) NDN_LOG_INFO(expr)
#define VSYNC_LOG_DEBUG(expr) NDN_LOG_DEBUG(expr)
#define VSYNC_LOG_WARN(expr) NDN_LOG_WARN(expr)
#define VSYNC_LOG_ERROR(expr) NDN_LOG_ERROR(expr)



*/

namespace ndn {
namespace vsync {

class Logger {
public:
  Logger(uint64_t nid);

  void
  disable();

  void
  enable();

  void
  logDataStore(const Name& name);

  void
  logStateStore(const NodeID& nid, int64_t seq);

  void
  logDist(double dist);

private:
  uint64_t m_nid;
  bool is_enabled;
};



} // namespace vsync
} // namespace ndn

#endif //NDN_VSYNC_LOGGING_HPP_
