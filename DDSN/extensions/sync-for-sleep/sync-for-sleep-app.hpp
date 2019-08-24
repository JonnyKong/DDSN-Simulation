#pragma once

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "ns3/ndnSIM/NFD/daemon/table/pit.hpp"

#include "sync-sleep-node.hpp"

using nfd::pit::Pit;

namespace ns3 {
namespace ndn {

namespace vsync = ::ndn::vsync;

class SyncForSleepApp : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = TypeId("SyncForSleepApp")
      .SetParent<Application>()
      .AddConstructor<SyncForSleepApp>()
      .AddAttribute("NodeID", "NodeID for sync node", UintegerValue(0),
                    MakeUintegerAccessor(&SyncForSleepApp::nid_), MakeUintegerChecker<uint64_t>())
      .AddAttribute("Prefix", "Prefix for sync node", StringValue("/"),
                    MakeNameAccessor(&SyncForSleepApp::prefix_), MakeNameChecker());
    return tid;
  }

  std::pair<double, double>
  GetCurrentPosition() {
    double cur_pos_x = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
    double cur_pos_y = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
    return std::make_pair(cur_pos_x, cur_pos_y);
  }

  int GetNumSurroundingNodes_() {
    int num = 0;
    for (NodeContainer::Iterator i = container_->Begin(); i != container_->End(); ++i) {
      if (*i == GetNode())
        continue;
      double distance = (GetNode() -> GetObject<MobilityModel>())
                        -> GetDistanceFrom((*i) -> GetObject<MobilityModel>());
      if (distance <= wifi_range + 1e-3)
        num += 1;
    }
    return num;
  }

  bool IsImportantData(uint64_t node_id) {
    // return (node_id % 4) == (nid_ % 4);
    // return (node_id % 2) == (nid_ % 2);
    return true;
  }

  NodeContainer *container_;  // Point back to container in simulator
  float wifi_range;           // Wifi range from simulator to calculate num of surrounding nodes


protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    m_instance.reset(new vsync::sync_for_sleep::SimpleNode(
      nid_,
      prefix_,
      std::bind(&SyncForSleepApp::IsImportantData, this, _1),
      std::bind(&SyncForSleepApp::GetCurrentPosition, this),
      std::bind(&SyncForSleepApp::GetNumSurroundingNodes_, this)
    ));
    m_instance->Start();
  }

  virtual void
  StopApplication()
  {
    m_instance->Stop();
    m_instance.reset();
  }

private:
  std::unique_ptr<vsync::sync_for_sleep::SimpleNode> m_instance;
  vsync::NodeID nid_;
  Name prefix_;
  bool useBeacon_;
  bool useBeaconSuppression_;
  bool useRetx_;
};

} // namespace ndn
} // namespace ns3
