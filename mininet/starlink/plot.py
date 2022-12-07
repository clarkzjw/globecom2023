import os

import csv
from matplotlib import pyplot as plt
import sys
from numpy import mean
filename = "2022_12_06_11_20_AM_metrics.csv"


def plot_ping_latency():
    with open(filename, 'r') as f:
        reader = csv.reader(f)

        latency_ms = []
        timestamp = []
        for row in reader:
            if row[0] == "starlink_dish_pop_ping_latency_seconds":
                pairs = row[1:]
                for p in pairs:
                    p = p.strip('][').split(', ')
                    latency_ms.append(float(p[1].strip('\'')) * 1000)
                    timestamp.append(float(p[0]))

    with open('latency.txt', 'w') as f:
        for item in latency_ms:
            f.write(str(item) + '\n')

    print("mean: {}".format(mean(latency_ms)))
    # plt.plot(latency_ms)
    # plt.show()


def plot_ping_drop_ratio():
    with open(filename, 'r') as f:
        reader = csv.reader(f)

        drop = []
        timestamp = []
        for row in reader:
            if row[0] == "starlink_dish_pop_ping_drop_ratio":
                pairs = row[1:]
                for p in pairs:
                    p = p.strip('][').split(', ')
                    drop.append(float(p[1].strip('\'')))
                    timestamp.append(float(p[0]))

    with open('drop.txt', 'w') as f:
        for item in drop:
            f.write(str(item) + '\n')

    print("mean: {}".format(mean(drop)))
    plt.plot(drop)
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(0)
    if sys.argv[1] == "ping":
        plot_ping_latency()
    elif sys.argv[1] == "ping_drop":
        plot_ping_drop_ratio()
