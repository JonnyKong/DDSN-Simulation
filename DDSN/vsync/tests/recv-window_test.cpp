/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */
/*
#include <boost/test/unit_test.hpp>

#include <string>

#include "recv-window.hpp"
#include "vsync-helper.hpp"

using ndn::vsync::ReceiveWindow;

BOOST_AUTO_TEST_SUITE(TestReceiveWindow);

BOOST_AUTO_TEST_CASE(Init) {
  ReceiveWindow rw;
  BOOST_CHECK_EQUAL(rw.Insert({{0, "A"}, 1}), false);
  BOOST_CHECK_EQUAL(rw.Insert({{1, ""}, 1}), false);
  BOOST_CHECK_EQUAL(rw.Insert({{1, "A"}, 0}), false);

  BOOST_CHECK_EQUAL(rw.Insert({{1, "A"}, 1}), true);
  BOOST_CHECK_EQUAL(rw.Insert({{1, "A"}, 3}), true);

  BOOST_CHECK_EQUAL(rw.Insert({{2, "B"}, 1}), true);

  BOOST_CHECK_EQUAL(rw.Insert({{2, "B1"}, 2}), false);
}

BOOST_AUTO_TEST_CASE(CheckForMissingData) {
  ReceiveWindow rw;
  rw.Insert({{1, "A"}, 1});
  rw.Insert({{1, "A"}, 3});
  auto p = rw.CheckForMissingData({{1, "A"}, 3}, {2, "A"});
  BOOST_CHECK_EQUAL(p.second, true);
  ReceiveWindow::SeqNumIntervalSet expected1;
  expected1.insert(ReceiveWindow::SeqNumInterval(2));
  BOOST_CHECK_EQUAL(p.first, expected1);

  p = rw.CheckForMissingData({{1, "A"}, 50}, {1, "A"});
  BOOST_CHECK_EQUAL(p.second, false);
  p = rw.CheckForMissingData({{1, "A"}, 3}, {2, "B"});
  BOOST_CHECK_EQUAL(p.second, false);
  p = rw.CheckForMissingData({{1, "A"}, 3}, {6, "A"});
  BOOST_CHECK_EQUAL(p.second, false);

  rw.Insert({{1, "A"}, 2});
  p = rw.CheckForMissingData({{1, "A"}, 3}, {2, "A"});
  BOOST_CHECK_EQUAL(p.second, true);
  ReceiveWindow::SeqNumIntervalSet expected2;
  BOOST_CHECK_EQUAL(p.first, expected2);

  expected2.insert(ReceiveWindow::SeqNumInterval::closed(1, 50));
  p = rw.CheckForMissingData({{2, "A"}, 50}, {3, "A"});
  BOOST_CHECK_EQUAL(p.second, true);
  BOOST_CHECK_EQUAL(p.first, expected2);

  rw.Insert({{2, "A"}, 10});
  rw.Insert({{2, "A"}, 11});
  p = rw.CheckForMissingData({{2, "A"}, 50}, {3, "A"});
  BOOST_CHECK_EQUAL(p.second, true);
  ReceiveWindow::SeqNumIntervalSet expected3;
  expected3.insert(ReceiveWindow::SeqNumInterval::right_open(1, 10));
  expected3.insert(ReceiveWindow::SeqNumInterval::closed(12, 50));
  BOOST_CHECK_EQUAL(p.first, expected3);
}

BOOST_AUTO_TEST_CASE(HasAllData) {
  ReceiveWindow rw;
  rw.Insert({{1, "A"}, 1});
  rw.Insert({{1, "A"}, 3});
  rw.CheckForMissingData({{1, "A"}, 3}, {2, "A"});
  BOOST_CHECK_EQUAL(rw.HasAllData({1, "A"}), false);
  BOOST_CHECK_EQUAL(rw.HasAllDataBefore({1, "A"}, 1), true);
  rw.Insert({{1, "A"}, 2});
  BOOST_CHECK_EQUAL(rw.HasAllData({1, "A"}), true);
  BOOST_CHECK_EQUAL(rw.HasAllDataBefore({1, "A"}, 3), true);

  rw.Insert({{2, "A"}, 1});
  rw.Insert({{2, "A"}, 2});
  rw.Insert({{2, "A"}, 3});
  rw.Insert({{2, "A"}, 4});
  rw.Insert({{2, "A"}, 11});
  rw.Insert({{2, "A"}, 12});
  BOOST_CHECK_EQUAL(rw.HasAllData({2, "A"}), false);
  BOOST_CHECK_EQUAL(rw.HasAllDataBefore({2, "A"}, 3), true);
  BOOST_CHECK_EQUAL(rw.HasAllDataBefore({2, "A"}, 4), true);
  BOOST_CHECK_EQUAL(rw.HasAllDataBefore({2, "A"}, 5), false);
}

BOOST_AUTO_TEST_CASE(LastAckedData) {
  ReceiveWindow rw;
  ndn::vsync::ESN esn{};
  rw.Insert({{1, "A"}, 1});
  rw.Insert({{1, "A"}, 3});
  esn = {{1, "A"}, 1};
  BOOST_CHECK_EQUAL(rw.LastAckedData(), esn);
  rw.Insert({{1, "A"}, 2});
  esn = {{1, "A"}, 3};
  BOOST_CHECK_EQUAL(rw.LastAckedData(), esn);
  rw.Insert({{2, "A"}, 1});
  rw.CheckForMissingData({{1, "A"}, 3}, {2, "A"});
  esn = {{2, "A"}, 1};
  BOOST_CHECK_EQUAL(rw.LastAckedData(), esn);
}

BOOST_AUTO_TEST_SUITE_END();
*/
