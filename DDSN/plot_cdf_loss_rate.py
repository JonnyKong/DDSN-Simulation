# -*- coding: utf-8 -*-
"""
    Plot CDFs under loss rates of 0, 0.05, 0.2, 0.5 in the given directories.
    @Author jonnykong@cs.ucla.edu
    @Date   2019-08-22
"""

from __future__ import print_function
import os
import sys
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

class CdfPlotter(object):
    """
    This class takes as input a list of filenames, calculate their average, and
    plot the averaged CDF, or save it to disk.
    """
    def __init__(self):
        """
        Initializes default x range to [0, 2400], and assume there are 20 nodes
        in total.
        """
        self._filenames = []
        self._dirs = [
           "chronosync_burst/results_0.0", 
           "chronosync_burst/results_0.05", 
           "chronosync_burst/results_0.2", 
           "chronosync_burst/results_0.50" 
           # "VectorSync/loss_rate_0",
           # "VectorSync/loss_rate_5",
           # "VectorSync/loss_rate_20",
           # "VectorSync/loss_rate_50"
           # "psync_burst/loss_rate_0",
           # "psync_burst/loss_rate_5",
           # "psync_burst/loss_rate_20",
           # "psync_burst/loss_rate_50"
        ]
        self._x_mesh = np.linspace(0, 2400, num=2400+1)
        self._node_num = 20
        self._ys_mesh = []   # 2D array
        
        self._ys_tmp = []
        for _, dir in enumerate(self._dirs):
            for i in range(12):
                self.add_file(os.path.join(dir, "result_%d.txt" % (i+1)))
            y_mean = [float(sum(l)) / len(l) for l in zip(*self._ys_mesh)]
            self._ys_tmp.append(y_mean)
            self._ys_mesh = []
        self._ys_mesh = self._ys_tmp
            

    def add_file(self, filename):
        """
        Add a new file and parse the output.
        """
        self._filenames.append(filename)
        self._parse_file(filename)

    def plot_cdf(self, save=False):
        """
        Draw CDF graph for every element in self._ys_mesh, or save the graph
        to "tmp.png" if specified.
        Args:
            save (bool): Save the output graph to disk, rather than displaying it
                on the screen (e.g. if you are working on a remote server).
        """
        font = {
            'family' : 'normal',
            'weight' : 'normal',
            'size'   : 14
        }
        matplotlib.rc('font', **font)
        
        fig = plt.figure()
        ax = fig.add_subplot(111)
        labels = [
            "loss rate = 0%", 
            "loss rate = 5%", 
            "loss rate = 20%", 
            "loss rate = 50%"
        ]
        styles = ['-', '--', '-.', ':']

        for i, ele in enumerate(self._ys_mesh):
            ax.plot(self._x_mesh, ele, 
                    label=labels[i], 
                    linestyle=styles[i],
                    linewidth=3)
        plt.ylim((0, 1))
        plt.xlim((0, 1000))
        plt.xlabel("Delay (s)")
        plt.ylabel("CDF")
        plt.legend(loc = 'center right', fancybox=True)
        # plt.legend(loc = 'lower right')
        plt.grid(True)
        plt.style.use('classic')
        if save:
            # fig.savefig('tmp.png')
            fig.set_size_inches(6, 3)
            fig.savefig('tmp.pdf', format='pdf', dpi=1000)
        else:
            plt.show()

    def _parse_file(self, filename, plot_delay=True):
        """
        Read one file and interpolate its values. Then, normalize the values
        to [0, 1] and add to self._ys_mesh.
        """
        x_coord = []
        data_store = set()
        generated_time = dict()
        with open(filename, "r") as f:
            for line in f.readlines():
                # if line.find("Store New Data") == -1:
                #     continue
                if line.find("Update New Seq") == -1:
                    continue
                elements = line.strip().split(' ')
                time = elements[0]
                data = elements[-1]
                if data not in data_store:
                    generated_time[data] = int(time)
                    data_store.add(data)
                if plot_delay:
                    x_coord.append((int(time) - generated_time[data]) / 1000000)
                else:
                    x_coord.append(int(time) / 1000000)
        x_coord.sort();
        y_mesh = self._interp0d(x_coord)
        y_mesh = [float(l / (len(data_store) * self._node_num)) for l in y_mesh]
        # y_mesh = [4 * float(i) for i in y_mesh]     # For partial data sync
        self._ys_mesh.append(y_mesh)
        print("Avail: %f" % y_mesh[-1])

    def _interp0d(self, x_coord):
        """
        0-d interpolation against self._x_mesh
        """
        y_interp0d = [0 for i in range(len(self._x_mesh))]
        for i, _ in enumerate(x_coord):
            y_interp0d[int(x_coord[i])] += 1
        for i in range(1, len(y_interp0d)):
            y_interp0d[i] += y_interp0d[i - 1]

        return y_interp0d

if __name__ == "__main__":
    plotter = CdfPlotter()
    plotter.plot_cdf(save=True)
