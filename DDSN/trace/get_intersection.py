    #!/usr/bin/env python
# coding:utf-8

"""
    @Author jonnykong@cs.ucla.edu
    @Date   2019-01-31
    Analyze the traces, and get the duration of two nodes intersect.
"""

import numpy as np
import sys
import re
import json
import matplotlib.pyplot as plt


filename = "scenario-20.ns_movements"


# Parse file
def read_trace(filename):
    result = dict()
    with open(filename, "r") as f:
        for line in f.readlines():
            if "set " in line:  # First occurence
                line = line.split()
                node = line[0].split('(')[1].split(')')[0]
                pos = float(line[3])
                if node not in result:
                    result[node] = dict()
                    result[node]["T"] = [float(0)]
                    result[node]["X"] = []
                    result[node]["Y"] = []
                if line[2] == "X_":
                    result[node]["X"].append(pos)
                elif line[2] == "Y_":
                    result[node]["Y"].append(pos)

            elif "setdest" in line:  # Subsequent occurence
                line = line.split()
                node = line[3].split('(')[1].split(')')[0]
                dest_x = float(line[5])
                dest_y = float(line[6])
                dest_speed = float(line[7].strip('\"'))
                time_cur = float(line[2])
                if (time_cur > 1e-5):  # time != 0
                    # Analyze the speed from last coordinate to current one
                    duration = time_cur - result[node]["T"][-1]
                    direction = np.arctan((result[node]["dest_y"] - result[node]["Y"][-1]) /
                                          (result[node]["dest_x"] - result[node]["X"][-1]))
                    if result[node]["dest_y"] > result[node]["Y"][-1] and direction < 0:
                        direction += np.pi
                    elif result[node]["dest_y"] < result[node]["Y"][-1] and direction > 0:
                        direction += np.pi

                    speed_x = result[node]["dest_speed"] * np.cos(direction)
                    speed_y = result[node]["dest_speed"] * np.sin(direction)

                    # If reached dest within this duration
                    if (np.abs(duration * speed_x) > np.abs((result[node]["dest_x"] - result[node]["X"][-1]))):
                        result[node]["X"].append(result[node]["dest_x"])
                        result[node]["Y"].append(result[node]["dest_y"])
                    else:
                        result[node]["X"].append(result[node]["X"][-1] + duration * speed_x)
                        result[node]["Y"].append(result[node]["Y"][-1] + duration * speed_y)
                    result[node]["T"].append(time_cur)

                # Return command at this step
                result[node]["dest_x"] = dest_x
                result[node]["dest_y"] = dest_y
                result[node]["dest_speed"] = dest_speed
    return result
            
# Get interpolated traces
def interpolate_trace(t, x, y, start, end, num):
    t_interpolated = np.linspace(start, end, num)
    x_interpolated = np.interp(t_interpolated, t, x)
    y_interpolated = np.interp(t_interpolated, t, y)
    return t_interpolated, x_interpolated, y_interpolated

# Get cdf of an array
def cdf(data):
    num_bins = 600
    values, base = np.histogram(data, bins = num_bins)
    cumulative = np.cumsum(values)
    max_ = cumulative[-1]
    cumulative = [i / max_ for i in cumulative]
    return base, cumulative

# Plot reverse cdf of many lists
def plot_cdf(lists):
    colors = ["b", "g", "r", "c", "m", "y"]
    lines = [None for i in range(6)]
    i = 0
    for list in lists:
        base, cumulative = cdf(list)
        cumulative = [i for i in cumulative]
        lines[i], = plt.plot(base[:-1], cumulative, linestyle='-', color=colors[i])
        i += 1
    plt.legend(lines, [i for i in range(60, 161, 20)])
    plt.xlabel("Duration of Contact/s (Mobile Nodes Only)")
    # plt.xlabel("Duration of isolation/s (Mobile Nodes Only)")
    # plt.xlabel("Duration of connecting to a new node/s (All nodes)")
    plt.ylabel("CDF")
    plt.grid(True)
    plt.show()


# Get list of intersections between node m and n
def get_intersections(traces, m, n, wifi_range):
    m_t, m_x, m_y = interpolate_trace(traces[str(m)]["T"], traces[str(m)]["X"], traces[str(m)]["Y"], 0, 2680, 26800)
    n_t, n_x, n_y = interpolate_trace(traces[str(n)]["T"], traces[str(n)]["X"], traces[str(n)]["Y"], 0, 2680, 26800)
    in_range = False
    intersection_start = 0  # Start time of this intersection
    durations = []          # List of intersection durations
    for i in range(0, len(m_t)):
        if (m_x[i] - n_x[i])**2 + (m_y[i] - n_y[i])**2 <= wifi_range**2:
            if not in_range:    # Start of an intersection
                in_range = True
                intersection_start = m_t[i]
        else:
            if in_range:    # End of an intersection
                in_range = False
                durations.append((intersection_start, m_t[i]))
    if in_range:
        durations.append((intersection_start, m_t[-1]))
    return durations
    # in_range = True
    # intersection_start = 0  # Start time of this intersection
    # durations = []          # List of intersection durations
    # for i in range(0, len(m_t)):
    #     if (m_x[i] - n_x[i])**2 + (m_y[i] - n_y[i])**2 <= wifi_range**2:
    #         if in_range:    # Start of an intersection
    #             in_range = False
    #             intersection_start = m_t[i]
    #     else:
    #         if not in_range:    # End of an intersection
    #             in_range = True
    #             durations.append((intersection_start, m_t[i]))
    # if in_range:
    #     durations.append((intersection_start, m_t[-1]))
    # return durations


# Get a list of durations that node is isolated
def get_isolation_duration(traces, n, wifi_range):
    # Get interpolated traces of all nodes at once
    t = dict()
    x = dict()
    y = dict()
    for node in traces:
        t[node], x[node], y[node] = interpolate_trace(traces[node]["T"], traces[node]["X"], traces[node]["Y"], 0, 2680, 26800)
    durations = []
    was_isolate = False
    time_start_isolate = 0
    for i in range(0, len(t[str(n)])):
        is_isolate = True
        for node in traces:
            if node == str(n):
                continue
            if (x[str(n)][i] - x[node][i])**2 + (y[str(n)][i] - y[node][i])**2 <= wifi_range**2:
                is_isolate = False
                break
        if not was_isolate and is_isolate:
            time_start_isolate = t[str(n)][i]
        elif was_isolate and not is_isolate:
            durations.append((time_start_isolate, t[str(n)][i]))
        was_isolate = is_isolate
    return durations

# Get a list of durations that a node encounters a new node
def get_reconnect(traces, n, wifi_range):
# Get interpolated traces of all nodes at once
    t = dict()
    x = dict()
    y = dict()
    was_connected = dict()
    time_last_new_connect = 0
    for node in traces:
        t[node], x[node], y[node] = interpolate_trace(traces[node]["T"], traces[node]["X"], traces[node]["Y"], 0, 2680, 26800)
        if node != str(n):
            was_connected[node] = True
    durations = []
    for i in range(0, len(t[str(n)])):
        new_connection = False
        for node in traces:
            if node == str(n):
                continue
            if (x[str(n)][i] - x[node][i])**2 + (y[str(n)][i] - y[node][i])**2 <= wifi_range**2:
                if not was_connected[node]:
                    new_connection = True
                    was_connected[node] = True
            else:
                was_connected[node] = False
        if new_connection:
            durations.append(t[str(n)][i] - time_last_new_connect)
            time_last_new_connect = t[str(n)][i]
    return durations

if __name__ == "__main__":
    traces = read_trace(filename)
    # Calculate contact duration
    durations_all = []
    for wifi_range in range(60, 161, 20):
        durations = []
        for i in range(0, 19):
            for j in range(i + 1, 20):
                duration_sample = get_intersections(traces, i, j, wifi_range)
                durations += [(i - j) for (j, i) in duration_sample]
        list.sort(durations)
        durations_all.append(durations)
    plot_cdf(durations_all)

    # # Calculate isolation time
    # durations_all = []
    # for wifi_range in range(60, 161, 20):
    #     durations = []
    #     for i in range(0, 20):  # Mobile nodes only
    #         duration = get_isolation_duration(traces, i, wifi_range)
    #         for start, end in duration:
    #             durations.append(end - start)
    #     list.sort(durations)
    #     durations_all.append(durations)
    # plot_cdf(durations_all)

    # # Calculate reconnect duration
    # durations_all = []
    # for wifi_range in range(60, 161, 20):
    #     durations = []
    #     for i in range(0, 44):  # All nodes
    #         # print(i)
    #         duration = get_reconnect(traces, i, wifi_range)
    #         durations += duration
    #     durations_all.append(durations)
    # plot_cdf(durations_all)
