#ifndef NDN_TEST_RANGE_NODE_HPP_
#define NDN_TEST_RANGE_NODE_HPP_

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

namespace ndn {
namespace test_range {

static const Name kTestRangePrefix = Name("/ndn/testRange");

class TestRangeNode {
 public:
  TestRangeNode(uint64_t nid) : 
    scheduler_(face_.getIoService()),
    nid_(nid),
    key_chain_(ns3::ndn::StackHelper::getKeyChain())
  {
    face_.setInterestFilter(
      kTestRangePrefix, std::bind(&TestRangeNode::OnTestRangeInterest, this, _2),
      [this](const Name&, const std::string& reason) {
        std::cout << "Failed to register testRange prefix: " << reason << std::endl;
      });
  }

  void Start() {
    /*
    scheduler_.scheduleEvent(time::milliseconds(nid_ * 100),
                             [this] { SendInterest(); });
    */
    if (nid_ == 0 || nid_ == 2) {
      // std::cout << "max data size = " << face_.getMaxNdnPacketSize() << std::endl;
      SendInterest();
    }
  }

  void SendInterest() {
    Name n = kTestRangePrefix;
    n.appendNumber(nid_);
    Interest interest(n);
    face_.expressInterest(interest, std::bind(&TestRangeNode::OnRemoteData, this, _2),
                          [](const Interest&, const lp::Nack&) {},
                          [](const Interest&) {});
    // time::system_clock::time_point send_interest_time = time::system_clock::now();
    // uint64_t milliseconds = time::toUnixTimestamp(send_interest_time).count();
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << "node(" << nid_ << ") Send interest name=" << n.toUri() << " current time = " << now << std::endl; 
  }

  void OnTestRangeInterest(const Interest& interest) {
    // time::system_clock::time_point cur_time = time::system_clock::now();
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << "node(" << nid_ << ") receives the interest, current time = " << now << std::endl;
    std::shared_ptr<Data> data = std::make_shared<Data>(interest.getName());
    std::string data_content = std::string(100, '*');
    data->setContent(reinterpret_cast<const uint8_t*>(data_content.data()),
                     data_content.size());
    key_chain_.sign(*data, signingWithSha256());
    face_.put(*data);
  }


  void OnRemoteData(const Data& data) {
    int64_t now = ns3::Simulator::Now().GetMicroSeconds();
    std::cout << "node(" << nid_ << ") Recv Data name=" << data.getName().toUri() << " current time = " << now << std::endl; 
  }

private:
  Face face_;
  Scheduler scheduler_;
  uint64_t nid_;
  KeyChain& key_chain_;
};

}  // namespace geo_forwarding
}  // namespace ndn

#endif