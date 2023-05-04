import glob
import json
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cycler
from collections import defaultdict

fig_legend_loc = "upper center"
fig_legend_param = (0.8, 0.35)
fig_legend_param2 = (0.5, 0.35)

name_labels = {
    "LinTS": "LinTS",
    "mininet_lints": "LinTS",
    "LinUCB": "LinUCB",
    "minRTT": "minRTT",
    "RR": "RR",
    "gcloud_lints": "LinTS",
    "gcloud_linucb": "LinUCB"
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
    "mininet_lints": "alpha",
    "LinUCB": "alpha",
    "minRTT": "minRTT",
    "RR": "RR",
    "gcloud_lints": "alpha",
    "gcloud_linucb": "alpha"
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
    "LinUCB": "--",
    "mininet_lints": ":",
    "gcloud_lints": "-",
    "gcloud_linucb": "--"
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

    "mininet_lints",
    # "LinUCB",
    # "minRTT",
    # "RR",
    # "gcloud_lints",
    # "gcloud_linucb"
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
    dir = "../result/{}/".format(algorithm)
    avg_bitrate = []
    for filename in glob.glob(dir + "*.json"):
        if parameter != -1:
            if str(parameter) in filename and "rebuffering_history_" not in filename:
                avg_bitrate.append(average_bitrate(filename))
        else:
            if "rebuffering_history_" not in filename:
                avg_bitrate.append(average_bitrate(filename))

    count, bins_count = np.histogram(avg_bitrate, bins=255)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def plot_average_bitrate_cdf():
    for alg in alg_list:
        alpha = -1
        if alg == "mininet_lints" or alg == "gcloud_lints":
            alpha = 1.0
        elif alg == "LinUCB":
            alpha = 0.2
        cdf, bins = calc_avg_bitrate_cdf(alg, alpha)
        if alpha != -1:
            # plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
            #          linestyle=linestyle[alg])
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
        else:
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
        # plt.legend()
    plt.figlegend(loc=fig_legend_loc, bbox_to_anchor=fig_legend_param2)
    # plt.title("Average Bitrate CDF, total segments = 200")
    plt.xlabel("Average Playback Bitrate (Kbps)")
    plt.ylabel("CDF")
    # plt.tight_layout()
    plt.xlim(0, 100000)
    plt.savefig("./average_bitrate_cdf.png")
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


def bitrate_change_count(filename: str) -> int:
    with open(filename, 'r') as f:
        history = json.load(f)
        history = sorted(history, key=lambda d: d['seg_no'])

        bitrate = -1
        count = -1

        for i in range(len(history)):
            if history[i]["initial_explore"] == 0:
                if history[i]['bitrate'] != bitrate:
                    count += 1
                    bitrate = history[i]['bitrate']
                # if history[i]['rebuffering_ratio_after'] - history[i]['rebuffering_ratio'] > 0:
                #     rebuffering += 1

        return count



def rebuffering_count_new(filename: str) -> int:
    with open(filename, 'r') as f:
        history = json.load(f)
        rebuffering = 0

        for i in range(len(history)):
            if history[i]["next_seg_no"] > 12:
                rebuffering += 1

        return rebuffering


def calc_rebuffering_count_cdf(algorithm: str, parameter: float):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    rebuffering_events_count = []
    for filename in glob.glob(dir + "*.json"):
        # print(filename)
        if parameter != -1:
            if str(parameter) in filename and "rebuffering_history_" not in filename:
                rebuffering_events_count.append(rebuffering_count(filename))
                # rebuffering_events_count.append(rebuffering_time_ratio(filename))
        else:
            if "rebuffering_history_" not in filename:
                rebuffering_events_count.append(rebuffering_count(filename))

        # if str(parameter) in filename and "rebuffering_history_" not in filename:
        #     rebuffering_events_count.append(rebuffering_count(filename))

    print(rebuffering_events_count)
    count, bins_count = np.histogram(rebuffering_events_count, bins=255)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def calc_bitrate_change_count_cdf(algorithm: str, parameter: float):
    if algorithm not in alg_list:
        print("results for algorithm " + algorithm + " not found")
        return
    dir = "../result/{}/".format(algorithm)
    bitrate_change_events_count = []
    for filename in glob.glob(dir + "*.json"):
        # print(filename)
        if parameter != -1:
            if str(parameter) in filename and "rebuffering_history_" not in filename:
                bitrate_change_events_count.append(bitrate_change_count(filename))
                # rebuffering_events_count.append(rebuffering_time_ratio(filename))
        else:
            if "rebuffering_history_" not in filename:
                bitrate_change_events_count.append(bitrate_change_count(filename))

        # if str(parameter) in filename and "rebuffering_history_" not in filename:
        #     rebuffering_events_count.append(rebuffering_count(filename))

    print(bitrate_change_events_count)
    count, bins_count = np.histogram(bitrate_change_events_count, bins=255)
    print(count)
    print(bins_count)
    pdf = count / sum(count)
    cdf = np.cumsum(pdf)
    return cdf, bins_count


def plot_rebuffering_count_cdf():
    for alg in alg_list:
        alpha = -1
        if alg == "LinTS":
            alpha = 0.2
        elif alg == "LinUCB":
            alpha = 0.2
        cdf, bins = calc_rebuffering_count_cdf(alg, alpha)
        if alpha != -1:
            # plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
            #          linestyle=linestyle[alg])
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
        else:
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
    plt.figlegend(loc=fig_legend_loc, bbox_to_anchor=fig_legend_param)

        # if alg == "LinTS" or alg == "LinUCB":
        #     for alpha in param_list:
        #         cdf, bins = calc_rebuffering_count_cdf(alg, alpha)
        #         if alpha != -1:
        #             plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
        #                      linestyle=linestyle[alg])
        #         else:
        #             plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
        #                      linestyle=linestyle[alg])
        #         plt.legend()
    # plt.title("Rebuffering Count CDF, total segments = 200")
    plt.xlabel("Rebuffering Event Counts")
    plt.ylabel("CDF")
    plt.xlim(0, 10)
    # plt.tight_layout()
    plt.savefig("./rebuffering_count_cdf.png")
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


def rebuffering_time_ratio(filename: str) -> float:
    with open(filename, 'r') as f:
        history = json.load(f)

        rebuffering = 0

        for i in range(len(history)):
            if history[i]["next_seg_no"] > 12:
                rebuffering += history[i]["end"] - history[i]["start"]

        return rebuffering / (history[-1]["end"] - history[0]["start"])


def plot_different_parameters():
    plt.rc('axes', prop_cycle=(cycler('color', list('rbgyk')) +
                               cycler('linestyle', ['-', '--', ':', '-.', '-'])))
    for alg in ["LinTS"]:
        for alpha in param_list:
            cdf, bins = calc_avg_bitrate_cdf(alg, alpha)
            plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha))

    plt.legend()
    plt.xlabel("Average Playback Bitrate (Kbps)")
    plt.ylabel("CDF")
    plt.tight_layout()
    plt.savefig("./average_bitrate_cdf_lints_parameters.png")
    plt.show()


def plot_bitrate_change_cdf():
    for alg in alg_list:
        alpha = -1
        if alg == "LinTS":
            alpha = 0.2
        elif alg == "LinUCB":
            alpha = 0.2
        cdf, bins = calc_bitrate_change_count_cdf(alg, alpha)
        if alpha != -1:
            # plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
            #          linestyle=linestyle[alg])
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
        else:
            plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
                     linestyle=linestyle[alg])
        plt.legend()

        # if alg == "LinTS" or alg == "LinUCB":
        #     for alpha in param_list:
        #         cdf, bins = calc_rebuffering_count_cdf(alg, alpha)
        #         if alpha != -1:
        #             plt.plot(bins[1:], cdf, label="{}, {}={}".format(name_labels[alg], parameter_labels[alg], alpha),
        #                      linestyle=linestyle[alg])
        #         else:
        #             plt.plot(bins[1:], cdf, label="{}".format(name_labels[alg]),
        #                      linestyle=linestyle[alg])
        #         plt.legend()
    # plt.title("Rebuffering Count CDF, total segments = 200")
    plt.xlabel("Bitrate Fluctuation")
    plt.ylabel("CDF")
    plt.ylim(0, 1)
    # plt.xlim(0, 10)
    plt.tight_layout()
    plt.savefig("./bitrate_change_cdf.png")
    plt.show()


if __name__ == "__main__":
    # plot_bitrate_change_cdf()
    # plot_different_parameters()
    plot_average_bitrate_cdf()
    plot_rebuffering_count_cdf()
    # plot_path_percentage()
