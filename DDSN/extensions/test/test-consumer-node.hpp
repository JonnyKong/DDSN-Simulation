#ifndef NDN_TEST_CONSUMER_NODE_HPP_
#define NDN_TEST_CONSUMER_NODE_HPP_

#include <functional>
#include <iostream>
#include <random>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/util/backports.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/signal.hpp>

#include "ns3/simulator.h"
#include "ns3/nstime.h"

namespace ndn {

NS_LOG_COMPONENT_DEFINE("ndn.testForwarding");

static const Name kTestPrefix = Name("/ndn/test");

class TestConsumerNode {
 public:
  TestConsumerNode(int nodeID):
    scheduler_(face_.getIoService()),
    key_chain_(ns3::ndn::StackHelper::getKeyChain())
  {
    index = 0;
    nid = nodeID;

    face_.setInterestFilter(
      kTestPrefix, std::bind(&TestConsumerNode::OnTestInterest, this, _2),
      [this](const Name&, const std::string& reason) {
        NS_LOG_INFO( "Failed to register test-interest: " << reason );
      });
  }

  void Start() {
    if (nid == 0) SendInterest();
  }

  void SendInterest() {
    scheduler_.scheduleEvent(time::nanoseconds(123456),
                             [this] { SendOutInterest(); });
  }


  void OnRemoteData(const Data& data) {
    // NS_LOG_INFO( "node(" << nid << ") receives the data!" );
  }

  void OnTestInterest(const Interest& interest) {
    NS_LOG_INFO ("node(" << nid << ") receives the test interest!" );
    std::cout << ns3::Simulator::Now().GetNanoSeconds() << " us node(" << nid << ") receives the test interest!" << std::endl;
    
    if (index == 1) return;
    index++;
    scheduler_.scheduleEvent(time::milliseconds(2),
                             [this] { SendOutInterest(); });
    /*
    if (nid == 1) {
      std::shared_ptr<Data> data = std::make_shared<Data>(interest.getName());
      data->setFreshnessPeriod(time::seconds(3600));
      data->setContent(make_shared< ::ndn::Buffer>(5));
      key_chain_.sign(*data, signingWithSha256());
      face_.put(*data);
    }
    */
  }

  void SendOutInterest() {
    Interest i(kTestPrefix, time::milliseconds(100));
    face_.expressInterest(i, std::bind(&TestConsumerNode::OnRemoteData, this, _2),
                          [](const Interest&, const lp::Nack&) {},
                          [](const Interest&) {});
    NS_LOG_INFO( "node(" << nid << ") sends out the test interest at!" );
    std::cout << ns3::Simulator::Now().GetNanoSeconds() << " us node(" << nid << ") sends out the test interest!" << std::endl;
  }

 private:
  uint32_t index;
  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;
  uint64_t nid;
};

}

#endif