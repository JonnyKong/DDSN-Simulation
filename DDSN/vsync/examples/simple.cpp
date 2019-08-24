/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include <functional>
#include <iostream>
#include <random>

#include "vsync.hpp"

namespace ndn {
namespace vsync {
namespace examples {

class SimpleNode {
 public:
  SimpleNode(const NodeID& nid, const Name& prefix, const ViewID& vid)
      : face_(io_service_),
        scheduler_(io_service_),
        node_(face_, scheduler_, key_chain_, nid, prefix, vid, 
          std::bind(&SimpleNode::OnData, this, _1, _2)),
        rengine_(rdevice_()),
        rdist_(500, 10000) {}

  void Start() {
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
    face_.processEvents();
  }

 private:
  void OnData(const std::string& content, const VersionVector& vv) {
    std::cout << "Upcall OnData: content=\"" << content << '"' << ", version vector=" << ToString(vv)
              << std::endl;
  }

  void PublishData() {
    node_.PublishData("Hello from " + node_.GetNodeID());
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
  }

  boost::asio::io_service io_service_;
  Face face_;
  Scheduler scheduler_;
  KeyChain key_chain_;
  Node node_;

  std::random_device rdevice_;
  std::mt19937 rengine_;
  std::uniform_int_distribution<> rdist_;
};

int main(int argc, char* argv[]) {
  // Create a simple view with three nodes
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " [node_id] [node_prefix] [view_id]"
              << std::endl;
    return -1;
  }

  NodeID nid = argv[1];
  Name prefix(argv[2]);
  ViewID vid = argv[3];

  SimpleNode node(nid, prefix, vid);
  node.Start();
  return 0;
}

}  // namespace examples
}  // namespace vsync
}  // namespace ndn

int main(int argc, char* argv[]) {
  return ndn::vsync::examples::main(argc, argv);
}
