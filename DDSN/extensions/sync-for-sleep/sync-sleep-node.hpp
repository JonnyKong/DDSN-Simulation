/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */
#pragma once

#include <functional>
#include <iostream>
#include <random>
#include "fstream"

#include "vsync.hpp"

#include <ndn-cxx/face.hpp>

namespace ndn {
namespace vsync {
namespace sync_for_sleep {

static const std::string snapshotFileName = "snapshot.txt";
class SimpleNode {
 public:

  SimpleNode(const NodeID& nid,
             const Name& prefix,
             Node::IsImportantData is_important_data,
             Node::GetCurrentPos getCurrentPos,
             Node::GetNumSurroundingNodes getNumSurroundingNodes)
      : scheduler_(face_.getIoService()),
        nid_(nid),
        // node_(face_, scheduler_, ns3::ndn::StackHelper::getKeyChain(), nid, prefix,
        //       std::bind(&SimpleNode::OnData, this, _1), getCurrentPos,
        //       useHeartbeat, useHeartbeatFlood, useBeacon, useBeaconSuppression, useRetx, useBeaconFlood)
        node_(face_,
              scheduler_,
              ns3::ndn::StackHelper::getKeyChain(),
              nid,
              prefix,
              std::bind(&SimpleNode::OnData, this, _1),
              is_important_data,
              getCurrentPos,
              getNumSurroundingNodes) {}

  void Start() {
  }

  void OnData(const VersionVector& vv) {
  }

  void Stop() {
    /*
    std::ofstream out;
    out.open(snapshotFileName, std::ofstream::out | std::ofstream::app);
    if (out.is_open()) {
      std::vector<uint64_t> data_snapshots = node_.GetDataSnapshots();
      std::vector<VersionVector> vv_snapshots = node_.GetVVSnapshots();
      std::vector<std::unordered_map<NodeID, ReceiveWindow>> rw_snapshots = node_.GetRWSnapshots();

      out << ToString(data_snapshots) << "\n";
      for (auto rw: rw_snapshots) {
        out << ToString(rw) << "\n";
      }
      for (auto vv: vv_snapshots) {
        out << ToString(vv) << "\n";
      }
    }
    else {
      std::cout << "Fail to write files" << std::endl;
    }
    */
  }

  std::string ToString(std::vector<uint64_t> list) {
    // if (list.size() == 0) return "";
    std::string res = "";
    for (uint64_t item: list) {
      res += to_string(item) + ",";
    }
    return res;
  }

  std::string ToString(std::unordered_map<NodeID, ReceiveWindow> rw_list) {
    std::string res = "";
    for (auto entry: rw_list) {
      NodeID nid = entry.first;
      ReceiveWindow rw = entry.second;
      ReceiveWindow::SeqNumIntervalSet win = rw.getWin();
      if (win.iterative_size() == 0) continue;
      auto it = win.begin();
      while (it != win.end()) {
        res += "(" + to_string(nid) + ":" + to_string(it->lower()) + "-" + to_string(it->upper()) + ")";
        it++;
      }
    }
    return res;
  }

  std::string ToString(VersionVector vv) {
    std::string res = "";
    for (auto entry: vv) {
      NodeID nid = entry.first;
      uint64_t seq = entry.second;
      res += "(" + to_string(nid) + ":" + to_string(seq) + ")";
    }
    return res;
  }

private:
  Face face_;
  Scheduler scheduler_;
  NodeID nid_;
  Node node_;
};

}  // sync_for_sleep
}  // namespace vsync
}  // namespace ndn

