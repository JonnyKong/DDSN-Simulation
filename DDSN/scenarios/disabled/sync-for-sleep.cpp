#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/netanim-module.h"

#include <map>

using namespace std;
using namespace ns3;

using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::L3RateTracer;
using ns3::ndn::FibHelper;

NS_LOG_COMPONENT_DEFINE ("ndn.SyncForSleep");

uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount, PhyRxBeginCount, PhyRxEndCount;

void
MacTxDrop(Ptr<const Packet> p)
{
  // NS_LOG_INFO("Packet Drop");
  MacTxDropCount++;
}

void
PrintDrop()
{
  std::cout << "******************** Collision Results ******************" << std::endl;
  std::cout << "MacTxDropCount: " << MacTxDropCount << std::endl;
  std::cout << "PhyTxDropCount: " << PhyTxDropCount << std::endl;
  std::cout << "PhyRxDropCount: " << PhyRxDropCount << std::endl;
  std::cout << "PhyRxBeginCount: " << PhyRxBeginCount << std::endl; 
  std::cout << "PhyRxEndCount: " << PhyRxEndCount << std::endl;
  std::cout << "******************** Collision Results ******************" << std::endl;
}

void
PhyTxDrop(Ptr<const Packet> p)
{
  // NS_LOG_INFO("Packet Drop");
  PhyTxDropCount++;
}
void
PhyRxDrop(Ptr<const Packet> p)
{
  // NS_LOG_INFO("Packet Drop");
  PhyRxDropCount++;
}
void
PhyRxBegin(Ptr<const Packet> p)
{
  PhyRxBeginCount++;
}
void
PhyRxEnd(Ptr<const Packet> p)
{
  PhyRxEndCount++;
}

//
// DISCLAIMER:  Note that this is an extremely simple example, containing just 2 wifi nodes communicating
//              directly over AdHoc channel.
//

// Ptr<ndn::NetDeviceFace>
// MyNetDeviceFaceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> device)
// {
//   // NS_LOG_DEBUG ("Create custom network device " << node->GetId ());
//   Ptr<ndn::NetDeviceFace> face = CreateObject<ndn::MyNetDeviceFace> (node, device);
//   ndn->AddFace (face);
//   return face;
// }

int
main (int argc, char *argv[])
{
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("OfdmRate24Mbps"));

  CommandLine cmd;
  cmd.Parse (argc,argv);

  //////////////////////
  //////////////////////
  //////////////////////
  WifiHelper wifi = WifiHelper::Default ();
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate24Mbps"));

  YansWifiChannelHelper wifiChannel;// = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::ThreeLogDistancePropagationLossModel");
  // wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");

  //YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default ();
  wifiPhyHelper.SetChannel (wifiChannel.Create ());
  wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
  wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));

  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  // constant mobility
  /*
  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable> ();
  randomizer->SetAttribute ("Min", DoubleValue (0));
  randomizer->SetAttribute ("Max", DoubleValue (800));

  Ptr<UniformRandomVariable> randomizerZ = CreateObject<UniformRandomVariable> ();
  randomizerZ->SetAttribute ("Min", DoubleValue (0));
  randomizerZ->SetAttribute ("Max", DoubleValue (0));

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                 "X", PointerValue (randomizer),
                                 "Y", PointerValue (randomizer),
                                 "Z", PointerValue (randomizerZ));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  */

  /*
  // RandomWalk
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("400.0"),
                                 "Y", StringValue ("400.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=400]"));
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (755.119, 560.052, 0.0));
  positionAlloc->Add (Vector (668.265, 73.9326, 0.0));
  positionAlloc->Add (Vector (61.8934, 325.192, 0.0));
  positionAlloc->Add (Vector (666.997, 754.103, 0.0));
  positionAlloc->Add (Vector (0.0876296, 244.216, 0.0));
  positionAlloc->Add (Vector (429.497, 320.051, 0.0));
  positionAlloc->Add (Vector (72.8159, 518.424, 0.0));
  positionAlloc->Add (Vector (168.832, 549.124, 0.0));
  positionAlloc->Add (Vector (756.332, 155.184, 0.0));
  positionAlloc->Add (Vector (751.312, 468.99, 0.0));
  positionAlloc->Add (Vector (672.91, 227.392, 0.0));
  positionAlloc->Add (Vector (640.308, 13.9314, 0.0));
  positionAlloc->Add (Vector (599.082, 726.259, 0.0));
  positionAlloc->Add (Vector (404.931, 533.498, 0.0));
  positionAlloc->Add (Vector (796.813, 152.158, 0.0));
  positionAlloc->Add (Vector (161.655, 171.424, 0.0));
  positionAlloc->Add (Vector (319.112, 233.189, 0.0));
  positionAlloc->Add (Vector (93.6488, 709.356, 0.0));
  positionAlloc->Add (Vector (113.604, 315.399, 0.0));
  positionAlloc->Add (Vector (759.217, 660.893, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("60s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"),
                             "Bounds", StringValue ("0|800|0|800"));
  */

  // new mpbility model
  // node 0, 1, 2, 3, 4 static, in one partition group A. node 5, 6, 7, 8 in group B. node 9 is moving between group A and group B
  MobilityHelper groupA;
  Ptr<ListPositionAllocator> positionAlloc_A = CreateObject<ListPositionAllocator> ();
  positionAlloc_A->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc_A->Add (Vector (100.0, 0.0, 0.0));
  positionAlloc_A->Add (Vector (200.0, 0.0, 0.0));
  positionAlloc_A->Add (Vector (300.0, 0.0, 0.0));
  positionAlloc_A->Add (Vector (400.0, 0.0, 0.0));
  groupA.SetPositionAllocator (positionAlloc_A);
  groupA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  MobilityHelper groupB;
  Ptr<ListPositionAllocator> positionAlloc_B = CreateObject<ListPositionAllocator> ();
  positionAlloc_B->Add (Vector (1000.0, 0.0, 0.0));
  positionAlloc_B->Add (Vector (1100.0, 0.0, 0.0));
  positionAlloc_B->Add (Vector (1200.0, 0.0, 0.0));
  positionAlloc_B->Add (Vector (1300.0, 0.0, 0.0));
  groupB.SetPositionAllocator (positionAlloc_B);
  groupB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  MobilityHelper mnode;
  mnode.SetMobilityModel("ns3::WaypointMobilityModel");

  // mobility.InstallAll ();

  NodeContainer nodes;
  nodes.Create (10);

  ////////////////
  // 1. Install Wifi
  NetDeviceContainer wifiNetDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, nodes);

  // 2. Install Mobility model
  // mobility.Install (nodes);
  for (int i = 0; i < 5; ++i) {
    groupA.Install (nodes.Get(i));
  }
  for (int i = 5; i < 9; ++i) {
    groupB.Install (nodes.Get(i));
  }
  mnode.Install (nodes.Get(9));
  Ptr<WaypointMobilityModel> wayMobility = nodes.Get(9)->GetObject<WaypointMobilityModel>();
  Vector pos1 = Vector (500.0, 0.0, 0.0);
  Vector pos2 = Vector (900.0, 0.0, 0.0);
  int flag = 1;
  for (int time = 0; time <= 600; time += 20) {
    Vector curVec;
    if (flag == 1) curVec = pos2;
    else curVec = pos1;
    flag *= -1;
    Waypoint cur(Seconds(time), curVec);
    wayMobility->AddWaypoint(cur);
  }

  // 3. Install NDN stack
  StackHelper ndnHelper;
  // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (MyNetDeviceFaceCallback));
  //ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll();

  // 4. Set Forwarding Strategy
  StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");

  // install SyncApp
  uint64_t idx = 0;
  for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); ++i) {
    Ptr<Node> object = *i;
    Ptr<MobilityModel> position = object->GetObject<MobilityModel>();
    Vector pos = position->GetPosition();
    std::cout << "node " << idx << " position: " << pos.x << " " << pos.y << std::endl;

    AppHelper syncForSleepAppHelper("SyncForSleepApp");
    syncForSleepAppHelper.SetAttribute("NodeID", UintegerValue(idx));
    syncForSleepAppHelper.SetAttribute("Prefix", StringValue("/"));
    auto app = syncForSleepAppHelper.Install(object);
    app.Start(Seconds(2));
    // app.Stop(Seconds (210.0 + idx));

    // StackHelper::setNodeID(idx, object);
    FibHelper::AddRoute(object, "/ndn/syncNotify", std::numeric_limits<int32_t>::max());
    FibHelper::AddRoute(object, "/ndn/vsyncData", std::numeric_limits<int32_t>::max());
    // FibHelper::AddRoute(object, "/ndn/heartbeat", std::numeric_limits<int32_t>::max());
    FibHelper::AddRoute(object, "/ndn/bundledData", std::numeric_limits<int32_t>::max());
    idx++;
  }

  // Trace Collisions
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback(&PhyRxBegin));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback(&PhyRxEnd));
  ////////////////

  Simulator::Stop (Seconds (600.0));

  // Simulator::Schedule(Seconds(200.0), &PrintDrop);

  std::string animFile = "ad-hoc-animation.xml";
  AnimationInterface anim (animFile);
  anim.SetMobilityPollInterval (Seconds (1));

  // L3RateTracer::InstallAll("test-rate-trace.txt", Seconds(0.5));
  // L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


