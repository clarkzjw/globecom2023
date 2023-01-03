import os

import csv

import numpy
from matplotlib import pyplot as plt
import sys
from numpy import mean
import matplotlib.dates as mdates
from matplotlib.dates import date2num
from datetime import datetime
import pytz

filename = "dec2022.csv"

pst_tz = pytz.timezone('US/Pacific')

def plot_ping():
    with open(filename, 'r') as f:
        reader = csv.reader(f)

        latency_ms = []
        drop = []
        latency_timestamp = []
        drop_timestamp = []
        for row in reader:
            if row[0] == "starlink_dish_pop_ping_latency_seconds":
                pairs = row[1:]
                for p in pairs:
                    p = p.strip('][').split(', ')
                    t = datetime.utcfromtimestamp(float(p[0])).astimezone(pst_tz)
                    if 18 <= t.hour <= 23 and t.month == 12:
                        latency_timestamp.append(t)
                        latency_ms.append(float(p[1].strip('\'')) * 1000)

            elif row[0] == "starlink_dish_pop_ping_drop_ratio":
                pairs = row[1:]
                for p in pairs:
                    p = p.strip('][').split(', ')
                    t = datetime.utcfromtimestamp(float(p[0])).astimezone(pst_tz)
                    if 18 <= t.hour <= 23 and t.month == 12:
                        drop_timestamp.append(t)
                        drop.append(float(p[1].strip('\'')))

    fig, axs = plt.subplots(2, 1)

    print("average ping latency: {}".format(mean(latency_ms)))
    print("median ping latency: {}".format(numpy.median(latency_ms)))
    print("max ping latency: {}".format(max(latency_ms)))
    print("stddev ping latency: {}".format(numpy.std(latency_ms)))
    print("\n")
    print("average ping drop ratio: {}".format(mean(drop)))
    print("median ping drop ratio: {}".format(numpy.median(drop)))
    print("max ping drop ratio: {}".format(max(drop)))
    print("stddev ping drop ratio: {}".format(numpy.std(drop)))

    xfmt = mdates.DateFormatter('%H:%M', tz=pst_tz)

    axs[0].plot(latency_timestamp, latency_ms)
    axs[0].set_title("Ping Latency (ms)")
    axs[0].xaxis.set_major_formatter(xfmt)

    axs[1].plot(drop_timestamp, drop)
    axs[1].set_title("Ping Drop Ratio")
    axs[1].xaxis.set_major_formatter(xfmt)

    plt.show()


def calculate_lte():
    lte_filename = "lte.txt"
    with open(lte_filename, 'r') as f:
        reader = csv.reader(f)

        latency_ms = []
        latency_timestamp = []
        for row in reader:
            print(row)
            row = row[0].split(" ")
            latency = row[1]
            timestamp = row[0]
            latency_ms.append(float(latency))
            latency_timestamp.append(datetime.utcfromtimestamp(float(timestamp)).astimezone(pst_tz))

        print("average ping latency: {}".format(mean(latency_ms)))
        print("median ping latency: {}".format(numpy.median(latency_ms)))
        print("max ping latency: {}".format(max(latency_ms)))
        print("stddev ping latency: {}".format(numpy.std(latency_ms)))


if __name__ == "__main__":
    # plot_ping()
    calculate_lte()
