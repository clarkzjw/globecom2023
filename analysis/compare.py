import glob
import json
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

name_labels = {
    "LinTS": "LinTS",
    "minRTT": "minRTT",
    "RR": "RR",
    # "lingreedy": "ε-Greedy",
    # "linucb": "UCB",
    # "lints_throughput_rtt_context_static": "LinTS (Static)",
    # "dynamic_lints_throughput_rtt_context": "LinTS (Dynamic)",
    # "dynamic_linucb_throughput_rtt_context": "LinUCB (Dynamic, Sigma = 5)",
    # "dynamic_lints_throughput_rtt_context_sigma_10": "LinTS (Dynamic, Sigma = 10)",
    # "dynamic_linucb_throughput_rtt_context_sigma_10": "LinUCB (Dynamic, Sigma = 10)",
    # "dynamic_lints_two_path_same_bw_mean_sigma_5": "LinTS (Dynamic, Same Bw Mean)",
    # "dynamic_linucb_two_path_same_bw_mean_sigma_5": "LinUCB (Dynamic, Same Bw Mean)",
    # "fluctuation_lints": "LinTS (Fluctuation)",
    # "new-rebuffering-lints": "LinTS (New Rebuffering)",
    # "dynamic_fluctuation_lints": "LinTS (Dynamic Fluctuation)",
    # "dynamic_fluctuation_linucb": "LinUCB (Dynamic Fluctuation)",
    # "dynamic_same_lints": "LinTS (Dynamic Same)",
    # "dynamic_same_linucb": "LinUCB (Dynamic Same)",
}

parameter_labels = {
    "LinTS": "alpha",
    "minRTT": "minRTT",
    "RR": "RR",
    # "lints_throughput_rtt_context_static": "alpha",
    # "lingreedy": "ε",
    # "linucb": "alpha",
    # "dynamic_lints_throughput_rtt_context": "alpha",
    # "dynamic_linucb_throughput_rtt_context": "alpha",
    # "dynamic_lints_throughput_rtt_context_sigma_10": "alpha",
    # "dynamic_linucb_throughput_rtt_context_sigma_10": "alpha",
    # "dynamic_lints_two_path_same_bw_mean_sigma_5": "alpha",
    # "dynamic_linucb_two_path_same_bw_mean_sigma_5": "alpha",
    # "fluctuation_lints": "alpha",
    # "new-rebuffering-lints": "alpha",
    # "new-rebuffering-lints-3": "alpha",
    # "dynamic_fluctuation_lints": "alpha",
    # "dynamic_fluctuation_linucb": "alpha",
    # "dynamic_same_lints": "alpha",
    # "dynamic_same_linucb": "alpha",
}


linestyle = {
    # "lints_throughput_rtt_context_static": ":",
    "LinTS": "-",
    "minRTT": ":",
    "RR": "-.",
    # "lingreedy": ":",
    # "linucb": "--",
    # "dynamic_lints_throughput_rtt_context": "-",
    # "dynamic_linucb_throughput_rtt_context": "--",
    # "dynamic_lints_throughput_rtt_context_sigma_10": "-.",
    # "dynamic_linucb_throughput_rtt_context_sigma_10": ":",
    # "dynamic_lints_two_path_same_bw_mean_sigma_5": "-.",
    # "dynamic_linucb_two_path_same_bw_mean_sigma_5": ":",
    # "fluctuation_lints": "-",
    # "new-rebuffering-lints": "-",
    # "new-rebuffering-lints-3": "-",
    # "dynamic_fluctuation_lints": ":",
    # "dynamic_fluctuation_linucb": "-",
    # "dynamic_same_lints": ":",
    # "dynamic_same_linucb": "-"
}

alg_list = [
    # "lints",
    # "lingreedy",
    # "linucb",
    # "lints_throughput_rtt_context_static",
    # "dynamic_lints_throughput_rtt_context",
    # "dynamic_linucb_throughput_rtt_context",
    # "dynamic_lints_throughput_rtt_context_sigma_10",
    # "dynamic_linucb_throughput_rtt_context_sigma_10",
    # "dynamic_lints_two_path_same_bw_mean_sigma_5",
    # "dynamic_linucb_two_path_same_bw_mean_sigma_5",
    # "fluctuation_lints"
    # "new-rebuffering-lints",
    # "new-rebuffering-lints-3"
    # "dynamic_fluctuation_lints",
    # "dynamic_fluctuation_linucb",
    # "dynamic_same_lints",
    # "dynamic_same_linucb"
    # "LinTS",
    "minRTT",
    "RR"
]

param_list = [
    0.1,
    0.5,
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


def calc_avg_bitrate_cdf(algorithm: str):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    avg_bitrate = []
    for filename in glob.glob(dir + "*.json"):
        if "rebuffering_history_" not in filename:
            avg_bitrate.append(average_bitrate(filename))

    count, bins_count = np.histogram(avg_bitrate)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def plot_average_bitrate_cdf():
    for alg in alg_list:
        cdf, bins = calc_avg_bitrate_cdf(alg)
        plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                 linestyle=linestyle[alg])
        plt.legend()
    plt.title("Average Bitrate CDF, total segments = 200")
    plt.xlabel("Average Bitrate in kbps")
    plt.tight_layout()
    plt.show()


"""
Rebuffering Count CDF
"""


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
            if history[i]["next_seg_no"] > 12:
                rebuffering += 1

        return rebuffering


def calc_rebuffering_count_cdf(algorithm: str):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    rebuffering_events_count = []
    for filename in glob.glob(dir + "*.json"):
        if "rebuffering_history_" not in filename:
            rebuffering_events_count.append(rebuffering_count(filename))

    print(rebuffering_events_count)
    count, bins_count = np.histogram(rebuffering_events_count)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def plot_rebuffering_count_cdf():
    for alg in alg_list:
        cdf, bins = calc_rebuffering_count_cdf(alg)
        plt.plot(bins[1:], cdf, label="{}, {}".format(name_labels[alg], parameter_labels[alg]),
                 linestyle=linestyle[alg])
        plt.legend()
    plt.title("Rebuffering Count CDF, total segments = 200")
    plt.xlabel("Rebuffering Event Counts in number of segments")
    plt.tight_layout()
    plt.show()


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
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    path1 = []
    path2 = []
    for filename in glob.glob(dir + "*.json"):
        if str(parameter) in filename:
            result = path_percentage(filename)
            path1.append(result[1])
            path2.append(result[2])

    return path1, path2


def plot_path_percentage():
    fig, axs = plt.subplots(2, 3, figsize=(10, 10))

    alpha_list = [0.1, 0.5, 1.0]
    for i in range(len(alg_list)):
        for j in range(len(alpha_list)):
            path1, path2 = calc_path_percentage(alg_list[i], alpha_list[j])
            axs[i, j].boxplot(path1)
            axs[i, j].boxplot(path2)
            axs[i, j].set_ylim(0, 1)
            axs[i, j].set_title("{}: {}".format(name_labels[alg_list[i]], parameter_labels[alg_list[i]] + " = " + str(alpha_list[j])))

    fig.suptitle("Path download percentage, total segments = 200")
    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    plot_average_bitrate_cdf()
    # plot_rebuffering_count_cdf()
    # plot_path_percentage()
