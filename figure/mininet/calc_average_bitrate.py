import glob
import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cycler
from collections import defaultdict

alg_list = [
    "LinTS",
    "LinUCB",
    "RR",
    "minRTT"
]


param_list = [
    0.2,
    0.4,
    0.6,
    0.8,
    1.0,
]

"""
Average bitrate CDF
"""


def average_bitrate(filename: str) -> float:
    with open(filename, 'r') as f:
        history = json.load(f)
        history = sorted(history, key=lambda d: d['seg_no'])

        bitrate = []

        for i in range(len(history)):
            if history[i]["initial_explore"] == 0:
                bitrate.append(history[i]['bitrate'])

        return sum(bitrate) / len(bitrate)


def calc_avg_bitrate_cdf(algorithm: str, parameter: float):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "./{}/".format(algorithm)
    avg_bitrate = []
    for filename in glob.glob(dir + "*.json"):
        if parameter != -1:
            if str(parameter) in filename and "rebuffering_history_" not in filename:
                avg_bitrate.append(average_bitrate(filename))
        else:
            if "rebuffering_history_" not in filename:
                avg_bitrate.append(average_bitrate(filename))

    return avg_bitrate


def calc_average_bitrate():
    for alg in alg_list:
        if alg in ["LinTS", "LinUCB"]:
            for alpha in param_list:
                avg_bitrate_list = calc_avg_bitrate_cdf(alg, alpha)
                print(avg_bitrate_list)
                with open("./{}-{}-average-bitrate.csv".format(alg, alpha), "w") as f:
                    for item in avg_bitrate_list:
                        f.write("%s\n" % item)
                print("done")
        else:
            avg_bitrate_list = calc_avg_bitrate_cdf(alg, -1)
            print(avg_bitrate_list)
            with open("./{}-average-bitrate.csv".format(alg), "w") as f:
                for item in avg_bitrate_list:
                    f.write("%s\n" % item)
            print("done")


def rebuffering_count(filename: str) -> int:
    with open(filename, 'r') as f:
        history = json.load(f)
        history = sorted(history, key=lambda d: d['seg_no'])

        rebuffering = 0

        for i in range(len(history)):
            if history[i]["initial_explore"] == 0:
                if history[i]['rebuffering_ratio_after'] - history[i]['rebuffering_ratio'] > 0:
                    rebuffering += 1

        return rebuffering

def rebuffering_count_new(filename: str) -> int:
    with open(filename, 'r') as f:
        history = json.load(f)
        rebuffering = 0

        for i in range(len(history)):
            # if history[i]["next_seg_no"] > 12:
            rebuffering += 1

        return rebuffering


def calc_rebuffering_count_cdf(algorithm: str, parameter: float):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "./{}/".format(algorithm)
    rebuffering_events_count = []
    for filename in glob.glob(dir + "*.json"):
        # print(filename)
        if parameter != -1:
            if str(parameter) in filename and "rebuffering_history_" not in filename:
                rebuffering_events_count.append(rebuffering_count(filename))
        else:
            if "rebuffering_history_" not in filename:
                rebuffering_events_count.append(rebuffering_count(filename))
    return rebuffering_events_count


def calc_rebuffering_count():
    for alg in alg_list:
        if alg in ["LinTS", "LinUCB"]:
            for alpha in param_list:
                avg_bitrate_list = calc_rebuffering_count_cdf(alg, alpha)
                print(avg_bitrate_list)
                with open("./{}-{}-rebuffering-count.csv".format(alg, alpha), "w") as f:
                    for item in avg_bitrate_list:
                        f.write("%s\n" % item)
                print("done")
        else:
            avg_bitrate_list = calc_rebuffering_count_cdf(alg, -1)
            print(avg_bitrate_list)
            with open("./{}-rebuffering-count.csv".format(alg), "w") as f:
                for item in avg_bitrate_list:
                    f.write("%s\n" % item)
            print("done")


if __name__ == "__main__":
    calc_average_bitrate()
    calc_rebuffering_count()
