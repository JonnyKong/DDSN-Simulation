#ifndef NDN_TEST_PRODUCER_NODE_HPP_
#define NDN_TEST_PRODUCER_NODE_HPP_

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

static const Name kTestPrefix = Name("/ndn/test");

class testProducerNode {
 public:
  testProducerNode():
    key_chain_(ns3::ndn::StackHelper::getKeyChain())
  {
    Name interest_register = Name(kTestPrefix);

    //std::cout << "Starting to register the geo-interest in geo-producer" << std::endl;
    face_.setInterestFilter(
      interest_register, std::bind(&testProducerNode::OnTestInterest, this, _2),
      [this](const Name&, const std::string& reason) {
        std::cout << "Failed to register geo-forwarding-producer: " << reason << std::endl;
      });
  }

  void Start() {
  }

  void OnTestInterest(const Interest& interest) {
    Name data_name(interest.getName());
    //size_t content_size = interest.getContent().value_size();
    //std::cout << "producer receives interest.Content size = " << content_size << std::endl;

    time::system_clock::time_point cur = time::system_clock::now();
    std::cout << to_string(time::system_clock::to_time_t(cur)) << std::endl;
    std::cout << "producer Recv interest " << interest.getName() << std::endl;
    /*
    std::shared_ptr<Data> data = std::make_shared<Data>(data_name);
    data->setFreshnessPeriod(time::seconds(3600));
    data->setContent(make_shared< ::ndn::Buffer>(5));
    key_chain_.sign(*data, signingWithSha256());
    face_.put(*data);
    std::cout << "producer Send Data" << std::endl;
    */
    
  }

private:
  Face face_;
  KeyChain& key_chain_;
};

}

#endif