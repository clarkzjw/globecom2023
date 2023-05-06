import glob
import json

import numpy
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

name_labels = {
    "lints": "Thompson Sampling",
    "lingreedy": "ε-Greedy",
    "linucb": "UCB",
}

parameter_labels = {
    "lints": "alpha",
    "lingreedy": "ε",
    "linucb": "alpha",
}

"""
Path download percentage
"""


def path_percentage(filename: str) -> dict:
    with open(filename, 'r') as f:
        history = json.load(f)
        history = sorted(history, key=lambda d: d['seg_no'])

        path_segment_count = defaultdict(int)

        for i in range(len(history)):
            if history[i]["initial_explore"] == 0:
                path_segment_count[history[i]['path_id']] += 1

        total = sum(path_segment_count.values())
        result = defaultdict(float)
        for key, value in path_segment_count.items():
            result[key] = value / total

        return result


def calc_path_percentage(algorithm: str, parameter: float) -> (list, list):
    if algorithm not in ["LinTS", "LinUCB", "RR", "minRTT"]:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "./{}/".format(algorithm)
    path1 = []
    path2 = []
    for filename in glob.glob(dir + "*.json"):
        if "rebuffering_" not in filename:
            result = path_percentage(filename)
            path1.append(result[1])
            path2.append(result[2])

    return path1, path2


def plot_path_percentage():
    alg_list = ["RR", "minRTT", "LinTS", "LinUCB"]
    path1_arr = []
    path2_arr = []
    for i in range(len(alg_list)):
        path1, path2 = calc_path_percentage(alg_list[i], 1.0)
        path1_arr.append(numpy.average(path1))
        path2_arr.append(numpy.average(path2))

    colors = {'path1': 'orange', 'path2': 'blue'}
    labels = list(colors.keys())
    handles = [plt.Rectangle((0, 0), 1, 1, color=colors[label]) for label in labels]
    plt.legend(handles, labels)

    x = np.arange(len(alg_list))
    plt.xticks(x, alg_list)
    plt.bar(range(len(path1_arr)), path1_arr, color="orange")
    plt.bar(range(len(path2_arr)), path2_arr, bottom=path1_arr, color="blue")
    plt.savefig('path_percentage.png', dpi=300, bbox_inches='tight')
    plt.show()


if __name__ == "__main__":
    plot_path_percentage()
