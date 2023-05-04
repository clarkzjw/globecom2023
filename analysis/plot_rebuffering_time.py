import glob
import json
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

name_labels = {
    "lints": "LinTS (context: playback buffer ratio)",
    "lingreedy": "ε-Greedy",
    "linucb": "UCB",
    "lints_throughput_rtt_context_static": "LinTS (Static)",
    "dynamic_lints_throughput_rtt_context": "LinTS (Dynamic)",
    "dynamic_linucb_throughput_rtt_context": "LinUCB (Dynamic, Sigma = 5)",
    "dynamic_lints_throughput_rtt_context_sigma_10": "LinTS (Dynamic, Sigma = 10)",
    "dynamic_linucb_throughput_rtt_context_sigma_10": "LinUCB (Dynamic, Sigma = 10)",
    "dynamic_lints_two_path_same_bw_mean_sigma_5": "LinTS (Dynamic, Same Bw Mean)",
    "dynamic_linucb_two_path_same_bw_mean_sigma_5": "LinUCB (Dynamic, Same Bw Mean)",
    "fluctuation_lints": "LinTS (Fluctuation)",
    "new-rebuffering-lints": "LinTS (New Rebuffering)",
    "dynamic_fluctuation_lints": "LinTS (Dynamic Fluctuation)",
    "dynamic_fluctuation_linucb": "LinUCB (Dynamic Fluctuation)",
    "dynamic_same_lints": "LinTS (Dynamic Same)",
    "dynamic_same_linucb": "LinUCB (Dynamic Same)",
}

parameter_labels = {
    "lints": "alpha",
    "lints_throughput_rtt_context_static": "alpha",
    "lingreedy": "ε",
    "linucb": "alpha",
    "dynamic_lints_throughput_rtt_context": "alpha",
    "dynamic_linucb_throughput_rtt_context": "alpha",
    "dynamic_lints_throughput_rtt_context_sigma_10": "alpha",
    "dynamic_linucb_throughput_rtt_context_sigma_10": "alpha",
    "dynamic_lints_two_path_same_bw_mean_sigma_5": "alpha",
    "dynamic_linucb_two_path_same_bw_mean_sigma_5": "alpha",
    "fluctuation_lints": "alpha",
    "new-rebuffering-lints": "alpha",
    "new-rebuffering-lints-3": "alpha",
    "dynamic_fluctuation_lints": "alpha",
    "dynamic_fluctuation_linucb": "alpha",
    "dynamic_same_lints": "alpha",
    "dynamic_same_linucb": "alpha",
}


linestyle = {
    "lints_throughput_rtt_context_static": ":",
    "lints": "-",
    "lingreedy": ":",
    "linucb": "--",
    "dynamic_lints_throughput_rtt_context": "-",
    "dynamic_linucb_throughput_rtt_context": "--",
    "dynamic_lints_throughput_rtt_context_sigma_10": "-.",
    "dynamic_linucb_throughput_rtt_context_sigma_10": ":",
    "dynamic_lints_two_path_same_bw_mean_sigma_5": "-.",
    "dynamic_linucb_two_path_same_bw_mean_sigma_5": ":",
    "fluctuation_lints": "-",
    "new-rebuffering-lints": "-",
    "new-rebuffering-lints-3": "-",
    "dynamic_fluctuation_lints": ":",
    "dynamic_fluctuation_linucb": "-",
    "dynamic_same_lints": ":",
    "dynamic_same_linucb": "-"
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
    "dynamic_fluctuation_lints",
    "dynamic_fluctuation_linucb",
    # "dynamic_same_lints",
    # "dynamic_same_linucb"
]

param_list = [
    0.1,
    0.5,
    1.0,
]


"""
Rebuffering Count CDF
"""

#
# def rebuffering_time_ratio(filename: str) -> int:
#     with open(filename, 'r') as f:
#         history = json.load(f)
#         history = sorted(history, key=lambda d: d['seg_no'])
#
#         rebuffering = 0
#
#         for i in range(len(history)):
#             if history[i]["initial_explore"] == 0:
#                 if history[i]['rebuffering_ratio_after'] - history[i]['rebuffering_ratio'] > 0:
#                     rebuffering += 1
#
#         return rebuffering


def rebuffering_time_ratio(filename: str) -> float:
    with open(filename, 'r') as f:
        history = json.load(f)

        rebuffering = 0

        for i in range(len(history)):
            if history[i]["next_seg_no"] > 12:
                rebuffering += history[i]["end"] - history[i]["start"]

        return rebuffering / (history[-1]["end"] - history[0]["start"])


def calc_rebuffering_time_cdf(algorithm: str, parameter: float):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    rebuffering_events_count = []
    for filename in glob.glob(dir + "*.json"):
        if str(parameter) in filename and "rebuffering_history_" in filename:
            rebuffering_events_count.append(rebuffering_time_ratio(filename) * 100)

    print(rebuffering_events_count)
    count, bins_count = np.histogram(rebuffering_events_count)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def plot_rebuffering_time_cdf():
    for alg in alg_list:
        for alpha in param_list:
            cdf, bins = calc_rebuffering_time_cdf(alg, alpha)
            plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
                     linestyle=linestyle[alg])
            plt.legend()
    plt.title("Rebuffering Count CDF, total segments = 200")
    plt.xlabel("Rebuffering Time Ratio (%)")
    plt.xlim(0, 100)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    plot_rebuffering_time_cdf()
