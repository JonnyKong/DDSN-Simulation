#include "logic.hpp"

#include "boost-test.hpp"

namespace chronosync {
namespace test {

using std::vector;


class Handler
{
public:
  Handler(ndn::Face& face,
          const Name& syncPrefix,
          const Name& userPrefix)
    : logic(face,
            syncPrefix,
            userPrefix,
            bind(&Handler::onUpdate, this, _1))
  {
  }

  void
  onUpdate(const vector<MissingDataInfo>& v)
  {

  }

  void
  updateSeqNo(const SeqNo& seqNo)
  {
    logic.updateSeqNo(seqNo);
  }

  void
  addUserNode(const Name& prefix)
  {
    logic.addUserNode(prefix);
  }

  void
  removeUserNode(const Name& prefix)
  {
    logic.removeUserNode(prefix);
  }


  Logic logic;
  std::map<Name, SeqNo> map;
};

class MultiUserFixture
{
public:

  MultiUserFixture()
    : syncPrefix("/ndn/broadcast/sync")
    , scheduler(io)
  {
    syncPrefix.appendVersion();
    userPrefix[0] = Name("/user0");
    userPrefix[1] = Name("/user1");
    userPrefix[2] = Name("/user2");

    face = make_shared<ndn::Face>(ref(io));
  }

  Name syncPrefix;
  Name userPrefix[3];

  boost::asio::io_service io;
  shared_ptr<ndn::Face> face;
  ndn::Scheduler scheduler;
  shared_ptr<Handler> handler;
};

BOOST_FIXTURE_TEST_SUITE(MultiUserTests, MultiUserFixture)

BOOST_AUTO_TEST_CASE(ThreeUserNode)
{
  handler = make_shared<Handler>(ref(*face), syncPrefix, userPrefix[0]);
  handler->addUserNode(userPrefix[1]);
  handler->addUserNode(userPrefix[2]);
  handler->removeUserNode(userPrefix[0]);

  handler->logic.setDefaultUserPrefix(userPrefix[1]);
  handler->updateSeqNo(1);
  BOOST_CHECK_EQUAL(handler->logic.getSeqNo(userPrefix[1]), 1);

  handler->logic.updateSeqNo(2, userPrefix[2]);
  handler->logic.setDefaultUserPrefix(userPrefix[2]);

  BOOST_CHECK_EQUAL(handler->logic.getSeqNo(), 2);

  BOOST_REQUIRE_THROW(handler->logic.getSeqNo(userPrefix[0]), Logic::Error);
  BOOST_REQUIRE_THROW(handler->logic.getSessionName(userPrefix[0]), Logic::Error);

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
