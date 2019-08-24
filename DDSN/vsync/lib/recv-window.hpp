/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef NDN_VSYNC_RECV_WINDOW_HPP_
#define NDN_VSYNC_RECV_WINDOW_HPP_

#include <iostream>
#include <iterator>
#include <map>
#include <string>

#include <boost/icl/interval_set.hpp>

#include "vsync-common.hpp"

namespace ndn {
namespace vsync {

class ReceiveWindow {
 public:
  using SeqNumInterval = boost::icl::discrete_interval<uint64_t>;
  using SeqNumIntervalSet = boost::icl::interval_set<uint64_t>;

  ReceiveWindow() {
    win.insert(0);
  }

  void Insert(uint64_t seq) {
    win.insert(SeqNumInterval(seq));
  }

  /**
   * @brief   Checks for missing data before sequence number @p seq.
   *
   * @param   seq  Sequence number to check
   * @return  An interval set of sequence numbers containing all the missing
   *          data.
   */
  // SeqNumIntervalSet CheckForMissingData(const uint64_t seq) {
  //   // Ignore sequence number 0.
  //   if (seq == 0) return {};

  //   SeqNumIntervalSet r;
  //   r.insert(SeqNumInterval::closed(1, seq));
  //   r -= win;
  //   return r;
  // }

  /**
   * @brief Given the last data info in @p ldi, checks for missing data in
   *        the same view as @p ldi.
   *
   * @param ldi  Last data info represented as ESN
   * @param vid  View ID of the view in which @p ldi was received
   */
  /*std::pair<SeqNumIntervalSet, bool> CheckForMissingData(const ESN& ldi,
                                                         const ViewID& vid) {
    // Ignore last data info with view/seq number equal to 0.
    if (ldi.vi.first == 0 || ldi.seq == 0) return {};

    // vid must be greater than ldi.vid.
    if (vid.first <= ldi.vi.first) return {};

    SeqNumIntervalSet r;
    r.insert(SeqNumInterval::closed(1, ldi.seq));
    auto& entry = state_[ldi.vi.first];
    if (!entry.leader_id.empty()) {
      if (entry.leader_id != ldi.vi.second) return {};
    } else {
      entry.leader_id = ldi.vi.second;
    }
    if (entry.last_seq_num != 0) {
      // Last data info is already set. Check if it is consistent with
      // the input parameters.
      if (entry.last_seq_num != ldi.seq) return {};
      if (entry.next_vi != vid) return {};
    } else {
      entry.last_seq_num = ldi.seq;
      entry.next_vi = vid;
    }
    r -= entry.win;
    return {r, true};
  }*/

  // bool HasAllData() const {
  //   return win.iterative_size() == 1;
  // }

  // bool HasAllDataBefore(uint64_t seq) const {
  //   return win.iterative_size() >= 1 && win.begin()->upper() >= seq;
  // }

  /**
   * Similar to TCP recv window.
   */
  uint64_t LastAckedData() const {
    if (win.empty())
      return 0;
    else
      return win.begin()->upper();
  }

  // uint64_t LastRecvData() const {
  //   if (win.empty())
  //     return 0;
  //   else {
  //     auto it = win.end();
  //     it--;
  //     return it->upper();
  //   }
  // }

  // bool HasData(uint64_t seq) {
  //   auto it = win.begin();
  //   while (it != win.end()) {
  //     if (seq >= it->lower() && seq <= it->upper()) return true;
  //     it++;
  //   }
  //   return false;
  // }

  SeqNumIntervalSet getWin() {
    return win;
  }

 private:
    SeqNumIntervalSet win;  // Intervals of received seq num

};

}  // namespace vsync
}  // namespace ndn

#endif  // NDN_VSYNC_RECV_WINDOW_HPP_
