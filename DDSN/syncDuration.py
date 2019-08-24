import os
import os.path
import numpy as np
# import pandas as pd
# import statsmodels.api as sm
# import matplotlib.pyplot as plt
import math
import re
import sys
import json, codecs

data_store = {}
state_store = {}

class DataInfo:
    def __init__(self, birth):
        self.GenerationTime = birth
        self.LastTime = birth
        self.Owner = 1
        self.Available = False

# def cdf_plot(data, name, number, c):
#   """
#   data: dataset
#   name: name on legend
#   number: how many pieces are split between min and max
#   """
#   ecdf = sm.distributions.ECDF(data)
#   x = np.linspace(min(data), max(data), number)
#   y = ecdf(x)

#   #plt.step(x, y, label=name)
#   plt.scatter(x, y, alpha=0.5, label=name, color=c)
#   #plt.xlim(0, 5)
#   plt.show()

# out_file_name = sys.argv[1]
syncDuration = []
stateSyncDuration = []
stateSyncTimestamp = []
out_notify_interest = []
out_beacon = []
out_data_interest = []
out_bundled_interest = []
out_data = []
out_ack = []
out_bundled_data = []
collision_rx = []
collision_tx = []
cache_hit = []
cache_hit_special = []
suppressed_sync_interest = []
file_name = sys.argv[1]
if int(sys.argv[2]) <= 5:
  node_num = int(sys.argv[2])
else:
  # node_num = int(sys.argv[2]) + 24
  node_num = int(sys.argv[2])
node_num_state_sync = 20    # For partial sync
recvDataReply = 0
recvForwardedDataReply = 0
recvSuppressedDataReply = 0
received_sync_interest = 0
should_receive_sync_interest = 0
data_reply = 0
received_data_mobile = 0
received_data_mobile_from_repo = 0
hibernate_duration = []

# file_name = "adhoc-log/syncDuration-movepattern.txt"
file = open(file_name)
for line in file:
    if line.find("microseconds") != -1:
      if line.find("Store New Data") != -1:
        elements = line.split(' ')
        time = elements[0]
        data_name = elements[-1]
        if data_name not in data_store:
            data_store[data_name] = DataInfo(int(time))
        else:
            data_store[data_name].Owner += 1
            data_store[data_name].LastTime = int(time)
        data_info = data_store[data_name]
        if data_info.Owner > node_num_state_sync:
            print line
            print data_name
            raise AssertionError()
        elif not data_info.Available and data_info.Owner == int(0.9 * node_num_state_sync):
            data_store[data_name].Available = True
            cur_sync_duration = data_info.LastTime - data_info.GenerationTime
            cur_sync_duration = float(cur_sync_duration) / 1000000.0
            syncDuration.append(cur_sync_duration)
      elif line.find("Update New Seq") != -1:
        elements = line.split(' ')
        time = elements[0]
        state_name = elements[-1]
        if state_name not in state_store:
            state_store[state_name] = DataInfo(int(time))
        else:
            state_store[state_name].Owner += 1
            state_store[state_name].LastTime = int(time)
        state_info = state_store[state_name]
        if state_info.Owner > node_num:
            print line
            print state_info.Owner
            raise AssertionError()
        elif not state_info.Available and state_info.Owner == int(0.9 * node_num):
            state_store[state_name].Available = True
            cur_sync_duration = state_info.LastTime - state_info.GenerationTime
            cur_sync_duration = float(cur_sync_duration) / 1000000.0
            stateSyncDuration.append(cur_sync_duration)
        stateSyncTimestamp.append(float(state_info.LastTime - state_info.GenerationTime) / 1000000.0)
    if line.find("NFD:") != -1:
      if line.find("m_outNotifyInterest") != -1:
        elements = line.split(' ')
        cur_out_notify_interest = elements[4][:-1]
        out_notify_interest.append(float(cur_out_notify_interest))
      if line.find("m_outBeacon") != -1:
        elements = line.split(' ')
        cur_out_beacon = elements[4][:-1]
        out_beacon.append(float(cur_out_beacon))
      elif line.find("m_outDataInterest") != -1:
        elements = line.split(' ')
        cur_out_data_interest = elements[4][:-1]
        out_data_interest.append(float(cur_out_data_interest))
      elif line.find("m_outBundledInterest") != -1:
        elements = line.split(' ')
        cur_out_bundled_interest = elements[4][:-1]
        out_bundled_interest.append(float(cur_out_bundled_interest))
      elif line.find("m_outData =") != -1:
        elements = line.split(' ')
        cur_out_data = elements[4][:-1]
        out_data.append(float(cur_out_data))
      elif line.find("m_outAck") != -1:
        elements = line.split(' ')
        cur_out_ack = elements[4][:-1]
        out_ack.append(float(cur_out_ack))
      elif line.find("m_outBundledData") != -1:
        elements = line.split(' ')
        cur_out_bundled_data = elements[4][:-1]
        out_bundled_data.append(float(cur_out_bundled_data))
    if line.find("CollisionRx") != -1:
      collision_rx.append(float(line.split(' ')[-1]))
    if line.find("CollisionTx") != -1:
      collision_tx.append(float(line.split(' ')[-1]))
    if line.find("m_cacheHit") != -1:
      cache_hit.append(float(line.split(' ')[-1]))
    if line.find("m_cacheHitSpecial") != -1:
      cache_hit_special.append(float(line.split(' ')[-1]))
    if line.find("received_sync_interest") != -1:
      received_sync_interest += int(line.split(' ')[-1])
    if line.find("suppressed_sync_interest") != -1:
      suppressed_sync_interest.append(float(line.split(' ')[-1]))
    if line.find("PhyRxDropCount") != -1:
      PhyRxDropCount = int(line.split()[-1])
    if line.find("Recv data reply") != -1:
      recvDataReply += 1
    if line.find("Recv forwarded data reply") != -1:
      recvForwardedDataReply += 1
    if line.find("Recv suppressed data reply") != -1:
      recvSuppressedDataReply += 1
    if line.find("should_receive_sync_interest") != -1:
      should_receive_sync_interest += int(line.split()[-1])
    if line.find("data_reply") != -1:
      data_reply += int(line.split()[-1])
    if line.find("received_data_mobile") != -1:
      received_data_mobile += int(line.split()[-1])
    if line.find("received_data_mobile_from_repo") != -1:
      received_data_mobile_from_repo += int(line.split()[-1])
    if line.find("hibernate_duration") != -1:
      hibernate_duration.append(float(line.split()[-1]))


# cdf_plot(syncDuration, "Synchronization Duration", 100, 'r')

# print the unsynced data
'''
print("The un-synced data:")
for data_name in data_store:
    if data_store[data_name].Owner != node_num:
        print(data_name)
'''

'''
with open(out_file_name, 'w') as outFile:
  outFile.write(str(syncDuration) + "\n")
  outFile.write("result for " + file_name + "\n")
  outFile.write("the number of synced data: " + str(len(syncDuration)) + "\n")
  outFile.write("percentage of complete sync: " + str(float(len(syncDuration)) / float(len(data_store))) + "\n")
  syncDuration = np.array(syncDuration)
  outFile.write("mean: " + str(np.mean(syncDuration)) + "\n")
  outFile.write("min: " + str(np.min(syncDuration)) + "\n")
  outFile.write("max: " + str(np.max(syncDuration)) + "\n")
  outFile.write("std: " + str(np.std(syncDuration)) + "\n")
'''

#print(data_store)
'''
print(str(syncDuration))
print("result for " + file_name)
print("the number of synced data: " + str(len(syncDuration)))
print("percentage of complete sync: " + str(float(len(syncDuration)) / float(len(data_store))))
syncDuration = np.array(syncDuration)
print("mean: " + str(np.mean(syncDuration)))
print("min: " + str(np.min(syncDuration)))
print("max: " + str(np.max(syncDuration)))
print("std: " + str(np.std(syncDuration)))
print("average outInterest: " + str(np.mean(np.array(out_interest))))
print("std outInterest: " + str(np.std(np.array(out_interest))))
print("average outData: " + str(np.mean(np.array(out_data))))
print("std outData: " + str(np.std(np.array(out_data))))
'''

print("data availability = " + str(float(len(syncDuration)) / float(len(data_store))))
stateSyncDuration = np.array(stateSyncDuration)
dataSyncDuration = np.array(syncDuration)
print("state sync delay = " + str(np.mean(stateSyncDuration)))
print("data sync delay = " + str(np.mean(syncDuration)))
print("out notify interest = " + str(np.sum(np.array(out_notify_interest))))
print("out beacon = " + str(np.sum(np.array(out_beacon))))
print("out data interest = " + str(np.sum(np.array(out_data_interest))))
print("out bundled interest = " + str(np.sum(np.array(out_bundled_interest))))
print("out data = " + str(np.sum(np.array(out_data))))
print("out ack = " + str(np.sum(np.array(out_ack))))
print("out bundled data = " + str(np.sum(np.array(out_bundled_data))))
# print("collision rx = " + str(np.mean(np.array(collision_rx))))
# print("collision tx = " + str(np.mean(np.array(collision_tx))))
print("cache hit = " + str(np.sum(np.array(cache_hit))))
print("cache hit special = " + str(np.sum(np.array(cache_hit_special))))

# for data_name in data_store:
#     print data_store[data_name].Owner

print("number of data available = " + str(dataSyncDuration.size))
print("number of data produced = " + str(len(data_store)))
print("recvDataReply = " + str(recvDataReply))
print("recvForwardedDataReply = " + str(recvForwardedDataReply))
print("recvSuppressedDataReply = " + str(recvSuppressedDataReply))
print("number of collision = " + str(PhyRxDropCount))
print("in notify interest = " + str(np.sum(received_sync_interest)))
print("suppressed notify interest = " + str(np.sum(suppressed_sync_interest)))
print("should receive interest = " + str(should_receive_sync_interest))
print("received interest = " + str(received_sync_interest))
print("collision rate = " + str(1 - float(received_sync_interest) / should_receive_sync_interest))
print("data reply = " + str(data_reply))
print("repo reply rate = " + str(float(received_data_mobile_from_repo) / received_data_mobile))
print("hibernate duration = " + str(np.mean(hibernate_duration)))


def cdf(data):
    data_size=len(data)

    # Set bins edges
    data_set=sorted(set(data))
    bins=np.append(data_set, data_set[-1]+1)

    # Use the histogram function to bin the data
    counts, bin_edges = np.histogram(data, bins=bins, density=False)

    counts=counts.astype(float)/data_size

    # Find the cdf
    cdf = np.cumsum(counts)

    # # Plot the cdf
    # plt.plot(bin_edges[0:-1], cdf,linestyle='--', marker="o", color='b')
    # plt.ylim((0,1))
    # plt.ylabel("CDF")
    # plt.grid(True)
    # plt.show()

    return (bin_edges, cdf)

# # Generate CDF plot
# cdf_file="cdf.txt"
# (bin_edges, cdf_result) = cdf(stateSyncTimestamp)
# cdf_string = json.dumps((bin_edges.tolist(), cdf_result.tolist()))
# with open(cdf_file, "a") as f:
#   f.write(cdf_string)
#   f.write("\n")
