/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef NDN_VSYNC_COMMON_HPP_
#define NDN_VSYNC_COMMON_HPP_

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/util/time.hpp>

#include "vsync-message.pb.h"

namespace ndn {
namespace vsync {

// Type and constant declarations for VectorSync

using NodeID = uint64_t;
using VersionVector = std::unordered_map<NodeID, uint64_t>;
using heartbeatVector = std::unordered_map<NodeID, uint64_t>;
using GroupID = std::string;

static const Name kSyncNotifyPrefix = Name("/ndn/syncNotify");
static const Name kSyncDataPrefix = Name("/ndn/vsyncData");
static const Name kGetNDNTraffic = Name("/ndn/getNDNTraffic");

typedef struct {
  std::shared_ptr<const Interest> interest;
  std::shared_ptr<const Data>     data;

  enum PacketType { INTEREST_TYPE, DATA_TYPE }        packet_type;
  enum SourceType { ORIGINAL, FORWARDED, SUPPRESSED } packet_origin;  // Used in data interest only

  int64_t last_sent_time;   // Timestamp when this packet was sent last time
  int64_t inf_retx_start_time;  // Timestamp when this packet entered inf retx queue
  float last_sent_dist;     // Distance recorded when this packet was last sent 
  int nRetries;
  int retransmission_counter;
  bool burst_packet;

} Packet;

}  // namespace vsync
}  // namespace ndn

#endif  // NDN_VSYNC_COMMON_HPP_
