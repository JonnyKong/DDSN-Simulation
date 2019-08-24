#ifndef NDN_TEST_PRODUCER_HPP_
#define NDN_TEST_PRODUCER_HPP_

#include "ns3/ndnSIM-module.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "test-producer-node.hpp"

namespace ns3 {

class testProducer : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = TypeId("testProducer")
      .SetParent<Application>()
      .AddConstructor<testProducer>();

    return tid;
  }

protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    m_instance.reset(new ::ndn::testProducerNode());
    m_instance->Start();
  }

  virtual void
  StopApplication()
  {
    m_instance.reset();
  }

private:
  std::unique_ptr<::ndn::testProducerNode> m_instance;
};

}

#endif