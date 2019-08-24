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

#ifndef CHRONOSYNC_LOGGER_HPP
#define CHRONOSYNC_LOGGER_HPP

#include <ndn-cxx/util/logger.hpp>

#define INIT_LOGGER(name) NDN_LOG_INIT(sync.name)

#define _LOG_ERROR(x) NDN_LOG_ERROR(x)
#define _LOG_WARN(x)  NDN_LOG_WARN(x)
#define _LOG_INFO(x)  NDN_LOG_INFO(x)
#define _LOG_DEBUG(x) NDN_LOG_DEBUG(x)
#define _LOG_TRACE(x) NDN_LOG_TRACE(x)

#define _LOG_FUNCTION(x)     NDN_LOG_TRACE(__FUNCTION__ << "(" << x << ")")
#define _LOG_FUNCTION_NOARGS NDN_LOG_TRACE(__FUNCTION__ << "()")

#endif // CHRONOSYNC_LOGGER_HPP
