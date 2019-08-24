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

#include "odometer.hpp"
#include <math.h>

namespace ndn {
namespace vsync {


Odometer::Odometer(std::function<std::pair<double, double>()> getCurrentPos_,
                   boost::asio::io_service& io_service_)
  : getCurrentPos(getCurrentPos_)
  , scheduler(io_service_)
  , distance_meter(0)
{
}

void
Odometer::init()
{
  std::tie(x_meter, y_meter) = getCurrentPos();
  scheduler.cancelEvent(running);
  running = scheduler.scheduleEvent(time::seconds(1), [this] {
    run();
  });
}

double
Odometer::getDist()
{
  return distance_meter;
}

void
Odometer::stop()
{
  scheduler.cancelEvent(running);
}

void
Odometer::run()
{
  double x_new, y_new;
  std::tie(x_new, y_new) = getCurrentPos();
  distance_meter += sqrt(pow(x_new - x_meter, 2) +
                         pow(y_new - y_meter, 2));
  x_meter = x_new;
  y_meter = y_new;
  running = scheduler.scheduleEvent(time::seconds(1), [this] {
    run();
  });
}


} // namespace vsync
} // namespace ndn
