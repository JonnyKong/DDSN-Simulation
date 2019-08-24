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

#ifndef NDN_VSYNC_ODOMETER_HPP_
#define NDN_VSYNC_ODOMETER_HPP_

#include "ndn-common.hpp"
#include "vsync-common.hpp"

namespace ndn {
namespace vsync {

/**
 * @brief Odometer logic for recording how much the node have travelled.
 *
 * NOTE: THIS LOGIC DOESN'T WORK OUTSIDE THE SIMULATOR
 *
 * Application need to pass in a callback to the simulator for the odometer
 * to query current location, and a scheduler to get the odometer running.
 *
 * The odometer logic querys current location every 1 second, and calculates
 * the approximate distance by assuming the node travelled in a straight line
 * from the last waypoint.
 */

class Odometer {
public:
  /**
   * @brief constructor
   *
   * @param getCurrentPos_ Callback to the simulator for current location
   * @param io_service_ The same io_service that the application uses
   */
  Odometer(std::function<std::pair<double, double>()> getCurrentPos_,
           boost::asio::io_service& io_service_);

  /**
   * @brief Start running the odometer. The odometer will keep scheduling
   * itself. Because it is assumed that the io_service used is the same one as
   * the application, we don't have to call face.processEvents()
   */
  void
  init();

  /**
   * @brief Stop running the odometer
   */
  void
  stop();

  /**
   * @ brief Get current distance travelled in meters
   */
  double
  getDist();


private:
  void
  run();

  std::function<std::pair<double, double>()> getCurrentPos;
  Scheduler scheduler;
  EventId running;
  double x_meter;
  double y_meter;
  double distance_meter;
};

} // namespace vsync
} // namespace ndn

#endif // NDN_VSYNC_ODOMETER_HPP_
