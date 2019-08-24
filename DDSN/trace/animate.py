import os, sys
import json

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

nodes_total = 21

# Read the ns-2 tracefile 
def read_trace(filename):
    result = dict()
    with open(filename, "r") as f:
        for line in f.readlines():
            # First occurence of a node
            if "set " in line:
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
            
            # Subsequent occurences of a node
            elif "setdest" in line:  
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



fig, ax = plt.subplots()
fig.subplots_adjust(left=0, right=1, bottom=0, top=1)
fig.subplots_adjust(left=0, right=1, bottom=0, top=1, wspace = 0.2, hspace = 0.2)
ax = fig.add_subplot(111, aspect='equal', autoscale_on=False,
                     xlim=(0, 800), ylim=(0, 800))
ax.grid()
xdata, ydata = [], []
t = []  # 1D array
x = []  # 2D array
y = []  # 2D array
# rect = plt.Rectangle((0, 0), 800, 800, ec='none', lw=2, fc='none')
# ax.add_patch(rect)
ln, = plt.plot([], [], 'ro', animated=True)
time_text = ax.text(0.43, 0.95, "", transform=ax.transAxes)

def init():
    global t
    global x
    global y
    # Get interpolated traces of all nodes
    filename = "scenario-20.ns_movements"
    traces = read_trace(filename)
    t, x_tmp, y_tmp = interpolate_trace(traces["0"]["T"], traces["0"]["X"], traces["0"]["Y"], 0, 2680, 26800)
    x.append(x_tmp)
    y.append(y_tmp)
    for i in range(1, nodes_total):
        _, x_tmp, y_tmp = interpolate_trace(traces[str(i)]["T"], traces[str(i)]["X"], traces[str(i)]["Y"], 0, 2680, 26800)
        x.append(x_tmp)
        y.append(y_tmp)
    # Transpose array
    x = [*zip(*x)]
    y = [*zip(*y)]
    xdata = x[0]
    ydata = y[0]
    time_text.set_text("")
    print(x)

    return ln, time_text

def update(frame):
    # xdata.append(frame)
    # ydata.append(np.sin(frame))
    xdata = x[frame]
    ydata = y[frame]
    time_text.set_text('time = %.1f' % t[frame])
    ln.set_data(xdata, ydata)
    return ln, time_text

ani = FuncAnimation(fig, update, 
                    # frames = np.linspace(0, 2*np.pi, 128),
                    frames = range(0, 26800),
                    init_func = init, 
                    blit = True,
                    interval = 10,
                    repeat = False)
# plt.show()
ani.save('21.mp4')