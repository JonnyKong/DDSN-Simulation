/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */
/*
#include <boost/test/unit_test.hpp>

#include <string>

#include <ndn-cxx/name.hpp>

#include "view-info.hpp"

using ndn::vsync::ViewInfo;

BOOST_AUTO_TEST_SUITE(TestViewInfo);

BOOST_AUTO_TEST_CASE(InitAndSize) {
  ViewInfo vinfo1;
  BOOST_CHECK_EQUAL(vinfo1.Size(), 0U);

  ViewInfo vinfo2({{"A", "/testA"}, {"B 1", "/testB"}, {"C", "/testC"}});
  BOOST_CHECK_EQUAL(vinfo2.Size(), 3U);

  BOOST_CHECK_THROW(ViewInfo({{"", "/"}}), ViewInfo::Error);

  std::string invalid(256, 'A');
  BOOST_CHECK_THROW(ViewInfo({{invalid, "/"}}), ViewInfo::Error);

  std::string invalid2("/");
  invalid2.append(invalid);
  BOOST_CHECK_THROW(ViewInfo({{"A", invalid2}}), ViewInfo::Error);
}

BOOST_AUTO_TEST_CASE(GetInfo) {
  ViewInfo vinfo({{"A", "/testA"}, {"B 1", "/testB"}, {"C", "/testC"}});

  auto p1 = vinfo.GetIndexByID("A");
  BOOST_CHECK_EQUAL(p1.first, 0U);
  BOOST_CHECK_EQUAL(p1.second, true);

  p1 = vinfo.GetIndexByID("B%201");
  BOOST_CHECK_EQUAL(p1.first, 1U);
  BOOST_CHECK_EQUAL(p1.second, true);

  p1 = vinfo.GetIndexByID("D");
  BOOST_CHECK_EQUAL(p1.second, false);

  auto p2 = vinfo.GetIDByIndex(2);
  BOOST_CHECK_EQUAL(p2.first, "C");
  BOOST_CHECK_EQUAL(p2.second, true);

  p2 = vinfo.GetIDByIndex(1);
  BOOST_CHECK_EQUAL(p2.first, "B%201");
  BOOST_CHECK_EQUAL(p2.second, true);

  p2 = vinfo.GetIDByIndex(5);
  BOOST_CHECK_EQUAL(p2.second, false);

  auto p3 = vinfo.GetPrefixByIndex(2);
  BOOST_CHECK_EQUAL(p3.first, ndn::Name("/testC"));
  BOOST_CHECK_EQUAL(p3.second, true);

  p3 = vinfo.GetPrefixByIndex(0);
  BOOST_CHECK_EQUAL(p3.first, ndn::Name("/testA"));
  BOOST_CHECK_EQUAL(p3.second, true);

  p3 = vinfo.GetPrefixByIndex(5);
  BOOST_CHECK_EQUAL(p3.second, false);
}

BOOST_AUTO_TEST_CASE(EncodeDecode) {
  ViewInfo original({{"A", "/testA"}, {"B 1", "/testB"}, {"C", "/testC"}});
  std::string out;
  original.Encode(out);

  ViewInfo vinfo;
  BOOST_TEST(vinfo.Decode(out.data(), out.size()));
  BOOST_TEST(vinfo == original);
}

BOOST_AUTO_TEST_CASE(Merge) {
  ViewInfo vinfo({{"A", "/testA"}, {"B", "/testB"}, {"C", "/testC"}});
  ViewInfo vinfo1({{"E", "/testE"}, {"D", "/testD"}});
  ViewInfo expected({{"A", "/testA"},
                     {"B", "/testB"},
                     {"C", "/testC"},
                     {"E", "/testE"},
                     {"D", "/testD"}});
  BOOST_CHECK_EQUAL(vinfo.Merge(vinfo1), true);
  BOOST_TEST(expected == vinfo);
  BOOST_CHECK_EQUAL(vinfo.Merge(vinfo1), false);
  BOOST_TEST(expected == vinfo);
}

BOOST_AUTO_TEST_CASE(Remove) {
  ViewInfo vinfo({{"A", "/testA"},
                  {"B", "/testB"},
                  {"C", "/testC"},
                  {"D", "/testD"},
                  {"E", "/testE"}});
  std::unordered_set<ndn::vsync::NodeID> to_be_removed{"B", "D"};
  ViewInfo expected({{"A", "/testA"}, {"C", "/testC"}, {"E", "/testE"}});
  vinfo.Remove(to_be_removed);
  BOOST_TEST(expected == vinfo);
  vinfo.Remove({"F"});
  BOOST_TEST(expected == vinfo);
}

BOOST_AUTO_TEST_SUITE_END();
*/