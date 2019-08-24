#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

 // Prints actual position and velocity when a course change event occurs
static void
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  NodeContainer nodes;
  nodes.Create (40);

  /*
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (1.0),
                                 "MinY", DoubleValue (1.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", RectangleValue (Rectangle (0.0, 20.0, 0.0, 20.0)));
  mobility.Install (nodes);
  */
  // Set mobility random number streams to fixed values
  // mobility.AssignStreams (sta, 0);

  /*
   // 2. Install Mobility model
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  int index = 0;
  for (int i = 0; i <= 300; i += 5) {
    positionAlloc->Add (Vector (double(i), 0.0, 0.0));
  }
  // <= 426, you can hear each other
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  mobility.AssignStreams (nodes, 0);
  */

  std::cout << "start to feed the trace file" << std::endl;
  auto traceFile = "scenario1.ns_movements";
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
  ns2.Install ();
  std::cout << "finishing feeding the trace file" << std::endl;

  uint64_t idx = 0;
  for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); ++i) {
    Ptr<Node> object = *i;

    Ptr<MobilityModel> position = object->GetObject<MobilityModel>();
    Vector pos = position->GetPosition();
    std::cout << "node " << idx << " position: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
    idx++;
  }

  std::ofstream os;
  os.open ("trace.txt");

  // Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
  //                  MakeBoundCallback (&CourseChange, &os));

  std::string animFile = "ad-hoc-animation.xml";
  AnimationInterface anim (animFile);

  Simulator::Stop (Seconds (2700));
  Simulator::Run ();
  Simulator::Destroy ();
}