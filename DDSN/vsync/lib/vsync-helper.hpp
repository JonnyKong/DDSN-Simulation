/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef NDN_VSYNC_INTEREST_HELPER_HPP_
#define NDN_VSYNC_INTEREST_HELPER_HPP_

#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>

#include <ndn-cxx/name.hpp>

#include "vsync-common.hpp"

namespace ndn {
namespace vsync {



inline std::string EncodeVVToName(const VersionVector& v) {
  std::string vv_encode = "";
  for (auto entry: v) {
    vv_encode += (to_string(entry.first) + "-" + to_string(entry.second) + "_");
  }
  return vv_encode;
}

/**
 * EncodeVVToNameWithInterest() - Encode version vector to name in format of :
 *  <NodeID>-<seq>-<interested>
 * Where interested is 0/1 indicating whether this node is interested in data
 *  produced by this NodeID.
 */
inline std::string
EncodeVVToNameWithInterest(const VersionVector &v,
                           std::function<bool(uint64_t)> is_important_data_,
                           std::unordered_map<NodeID, EventId> surrounding_producers) {
  std::string vv_encode = "";
  for (auto entry : v) {
    vv_encode += (to_string(entry.first) + "-" +
                  to_string(entry.second) + "-");
    // if (is_important_data_(entry.first) ||
    //     surrounding_producers.find(entry.first) != surrounding_producers.end())
    if (is_important_data_(entry.first))
      vv_encode += "1_";
    else
      vv_encode += "0_";
  }
  return vv_encode;
}


inline VersionVector DecodeVVFromName(const std::string& vv_encode) {
  int start = 0;
  VersionVector vv;
  for (size_t i = 0; i < vv_encode.size(); ++i) {
    if (vv_encode[i] == '_') {
      std::string str = vv_encode.substr(start, i - start);
      size_t sep = str.find("-");
      NodeID nid = std::stoull(str.substr(0, sep));
      uint64_t seq = std::stoull(str.substr(sep + 1));
      vv[nid] = seq;
      start = i + 1;
    }
  }
  return vv;
}

/**
 * DecodeVVFromNameWithInterest() - Given an encoded state vector encoded as:
 *  <NodeID>-<seq>-<interested>
 * Return the state vector, and a set of its interested nodes.
 */
inline std::pair<VersionVector, std::set<NodeID>>
DecodeVVFromNameWithInterest(const std::string& vv_encode) {
  int start = 0;
  VersionVector vv;
  std::set<NodeID> interested_nodes;
  for (size_t i = 0; i < vv_encode.size(); ++i) {
    if (vv_encode[i] == '_') {
      std::string str = vv_encode.substr(start, i - start);
      size_t cursor_1 = str.find("-");
      size_t cursor_2 = str.find("-", cursor_1 + 1);
      NodeID nid = std::stoll(str.substr(0, cursor_1));
      uint64_t seq = std::stoll(str.substr(cursor_1 + 1, cursor_2));
      bool is_important = std::stoll(str.substr(cursor_2 + 1));
      if (is_important)
        interested_nodes.insert(nid);
      vv[nid] = seq;
      start = i + 1;
    }
  }
  return std::make_pair(vv, interested_nodes);
}

inline void EncodeVV(const VersionVector& v, proto::VV* vv_proto) {
  for (auto item: v) {
    auto* entry = vv_proto->add_entry();
    entry->set_nid(item.first);
    entry->set_seq(item.second);
  }
}

inline void EncodeVV(const VersionVector& v, std::string& out) {
  proto::VV vv_proto;
  EncodeVV(v, &vv_proto);
  vv_proto.AppendToString(&out);
}

inline void 
EncodeVVWithInterest(const VersionVector& v,
                     proto::VV* vv_proto,
                     std::function<bool(uint64_t)> is_important_data_,
                     std::unordered_map<NodeID, EventId> surrounding_producers)
{
  for (auto item : v) {
    auto* entry = vv_proto->add_entry();
    entry->set_nid(item.first);
    entry->set_seq(item.second);
    // if (is_important_data_(item.first) ||
    //     surrounding_producers.find(item.first) != surrounding_producers.end())
    if (is_important_data_(item.first))
      entry->set_interested(true);
    else
      entry->set_interested(false);
  }
}

inline VersionVector DecodeVV(const proto::VV& vv_proto) {
  VersionVector vv;
  for (int i = 0; i < vv_proto.entry_size(); ++i) {
    const auto& entry = vv_proto.entry(i);
    auto nid = entry.nid();
    auto seq = entry.seq();
    vv[nid] = seq;
  }
  return vv;
}

inline VersionVector DecodeVV(const void* buf, size_t buf_size) {
  proto::VV vv_proto;
  if (!vv_proto.ParseFromArray(buf, buf_size)) {
    VersionVector res;
    return res;
  }
  return DecodeVV(vv_proto);
}

inline std::pair<VersionVector, std::set<NodeID>>
DecodeVVWithInterest(const proto::VV& vv_proto)
{
  VersionVector vv;
  std::set<NodeID> interested_nodes;
  for (int i = 0; i < vv_proto.entry_size(); ++i) {
    const auto& entry = vv_proto.entry(i);
    auto nid = entry.nid();
    auto seq = entry.seq();
    auto interested = entry.interested();
    vv[nid] = seq;
    if (interested)
      interested_nodes.insert(nid);
  }
  return std::make_pair(vv, interested_nodes);
}

// Naming conventions for interests and data
// TBD
// actually, the [state-vector] is no needed to be carried because the carried data contains the vv.
// but lixia said we maybe should remove the vv from data.
inline Name MakeSyncNotifyName(const NodeID& nid, std::string encoded_vv, int64_t timestamp) {
  // name = /[syncNotify_prefix]/[nid]/[state-vector]/[heartbeat-vector]
  Name n(kSyncNotifyPrefix);
  n.appendNumber(nid).append(encoded_vv).appendNumber(timestamp);
  return n;
}

inline Name MakeDataName(const NodeID& nid, uint64_t seq) {
  // name = /[vsyncData_prefix]/[node_id]/[seq]/%0
  Name n(kSyncDataPrefix);
  n.appendNumber(nid).appendNumber(seq).appendNumber(0);
  return n;
}

inline uint64_t ExtractNodeID(const Name& n) {
  return n.get(-3).toNumber();
}

inline std::string ExtractEncodedVV(const Name& n) {
  return n.get(-2).toUri();
}

inline std::string ExtractEncodedMV(const Name& n) {
  return n.get(-2).toUri();
}

inline uint64_t ExtractSequence(const Name& n) {
  return n.get(-2).toNumber();
}

inline std::string ExtractEncodedHV(const Name& n) {
  return n.get(-2).toUri();
}

inline std::string ExtractTag(const Name& n) {
  return n.get(-1).toUri();
}

inline uint64_t ExtractBeaconSender(const Name& n) {
  return n.get(-3).toNumber();
}

inline uint64_t ExtractBeaconInitializer(const Name& n) {
  return n.get(-2).toNumber();
}

inline uint64_t ExtractBeaconSeq(const Name& n) {
  return n.get(-1).toNumber();
}

}  // namespace vsync
}  // namespace ndn

#endif  // NDN_VSYNC_INTEREST_HELPER_HPP_
