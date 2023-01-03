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

# filename = "2022_12_06_11_20_AM_metrics.csv"
filename = "2023_01_02_02_58_PM_metrics.csv"


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
                    # timestamp.append(float(p[0]))
                    timestamp.append(date2num(datetime.utcfromtimestamp(float(p[0]))))

    with open('latency.txt', 'w') as f:
        for item in latency_ms:
            f.write(str(item) + '\n')

    # print("mean: {}".format(mean(latency_ms)))
    # plt.plot(latency_ms)
    # plt.show()
    fig, ax = plt.subplots()

    pst_tz = pytz.timezone('US/Pacific')
    pst_timestamp = [dt.astimezone(pst_tz) for dt in mdates.num2date(timestamp)]

    range_start = mdates.num2date(date2num(datetime.utcfromtimestamp(1672624800))).astimezone(pst_tz)
    range_end = mdates.num2date(date2num(datetime.utcfromtimestamp(1672642800))).astimezone(pst_tz)

    latency_night = []
    timestamp_night = []
    for i in range(len(pst_timestamp)):
        if range_start <= pst_timestamp[i] <= range_end:
            latency_night.append(latency_ms[i])
            timestamp_night.append(timestamp[i])

    print("average ping latency: {}".format(mean(latency_ms)))
    print("median ping latency: {}".format(numpy.median(latency_ms)))
    print("max ping latency: {}".format(max(latency_ms)))
    print("variance ping latency: {}".format(numpy.var(latency_ms)))

    print("average ping latency night: {}".format(mean(latency_night)))
    print("median ping latency night: {}".format(numpy.median(latency_night)))
    print("max ping latency night: {}".format(max(latency_night)))
    print("variance ping latency night: {}".format(numpy.var(latency_night)))

    ax.plot(timestamp_night, latency_night)

    xfmt = mdates.DateFormatter('%H:%M', tz=pst_tz)
    ax.xaxis.set_major_formatter(xfmt)

    plt.show()


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
                    timestamp.append(date2num(datetime.utcfromtimestamp(float(p[0]))))

    with open('drop.txt', 'w') as f:
        for item in drop:
            f.write(str(item) + '\n')

    fig, ax = plt.subplots()

    pst_tz = pytz.timezone('US/Pacific')
    pst_timestamp = [dt.astimezone(pst_tz) for dt in mdates.num2date(timestamp)]

    range_start = mdates.num2date(date2num(datetime.utcfromtimestamp(1672624800))).astimezone(pst_tz)
    range_end = mdates.num2date(date2num(datetime.utcfromtimestamp(1672642800))).astimezone(pst_tz)

    drop_night = []
    timestamp_night = []
    for i in range(len(pst_timestamp)):
        if range_start <= pst_timestamp[i] <= range_end:
            drop_night.append(drop[i])
            timestamp_night.append(timestamp[i])
    # timestamps_night = [t for t in pst_timestamp if range_start < t < range_end]

    print("mean: {}".format(mean(drop)))
    print("max ping drop: {}".format(max(drop)))
    print("variance: {}".format(numpy.var(drop)))

    print("mean night: {}".format(mean(drop_night)))
    print("max ping drop night: {}".format(max(drop_night)))
    print("variance night: {}".format(numpy.var(drop_night)))

    ax.plot(timestamp_night, drop_night)

    xfmt = mdates.DateFormatter('%H:%M', tz=pst_tz)
    ax.xaxis.set_major_formatter(xfmt)

    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(0)
    if sys.argv[1] == "ping":
        plot_ping_latency()
    elif sys.argv[1] == "ping_drop":
        plot_ping_drop_ratio()
