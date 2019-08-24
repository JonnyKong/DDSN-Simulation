/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {
namespace ndn {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     ./waf --run=chronosync-simple
 */

int
main(int argc, char *argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxPackets", UintegerValue(20));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/ndn", "ndn:/localhost/nfd/strategy/multicast");

  // Installing applications

  // Peer 1
  ndn::AppHelper peer1("ChronoSyncApp");
  peer1.SetAttribute("NodeID", UintegerValue(1));
  peer1.SetAttribute("SyncPrefix", StringValue("/ndn/broadcast/sync"));
  peer1.SetAttribute("UserPrefix", StringValue("/peer1"));
  peer1.SetAttribute("RoutingPrefix", StringValue("/ndn"));
  peer1.SetAttribute("MinNumberMessages", StringValue("1"));
  peer1.SetAttribute("MaxNumberMessages", StringValue("100"));
  peer1.SetAttribute("PeriodicPublishing", StringValue("true"));
  peer1.SetAttribute("DataGenerationDuration", IntegerValue(800));
  peer1.Install(nodes.Get(0)).Start(Seconds(2));

  // Peer 2
  ndn::AppHelper peer2("ChronoSyncApp");
  peer2.SetAttribute("NodeID", UintegerValue(2));
  peer2.SetAttribute("SyncPrefix", StringValue("/ndn/broadcast/sync"));
  peer2.SetAttribute("UserPrefix", StringValue("/peer2"));
  peer2.SetAttribute("RoutingPrefix", StringValue("/ndn"));
  peer2.SetAttribute("MinNumberMessages", StringValue("1"));
  peer2.SetAttribute("MaxNumberMessages", StringValue("100"));
  // peer2.SetAttribute("PeriodicPublishing", StringValue("true"));
  peer2.SetAttribute("DataGenerationDuration", IntegerValue(800));
  peer2.Install(nodes.Get(2)).Start(Seconds(2));

  // Manually configure FIB routes
  ndn::FibHelper::AddRoute(nodes.Get(0), "/ndn/broadcast/sync", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), "/ndn/broadcast/sync", nodes.Get(2), 1);
  ndn::FibHelper::AddRoute(nodes.Get(2), "/ndn/broadcast/sync", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), "/ndn/broadcast/sync", nodes.Get(0), 1);

  Simulator::Stop(Seconds(2400.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ndn
} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::ndn::main(argc, argv);
}
