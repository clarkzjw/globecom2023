import os
import shutil
import time
import numpy
from ctypes import *
from util import *
from multiprocessing import Process, Manager
import pandas as pd
from sklearn.preprocessing import StandardScaler
from mabwiser.mab import MAB, LearningPolicy, NeighborhoodPolicy
from collections import defaultdict

from mpd import parse_mpd
from bitrate import get_initial_bitrate, get_bitrate_level, get_max_bitrate, get_nb_bitrates, get_resolution
from bitrate import bitrate_mapping, build_arms
from config import playback_buffer_map, max_playback_buffer_size, rebuffering_ratio, history
from config import nb_paths, playback_buffer_frame_ratio, DEBUG_SEGMENTS
import config

default_mpd_url = "/home/clarkzjw/Documents/dataset/mpd/stream.mpd"
default_host = "10.0.1.2"
default_port = 443
tmp_dir = "./tmp"

DEBUG_WAIT = False
DEBUG_WAIT_SECONDS = 10
DEBUG = True

if_name_mapping = {
    1: "h2-eth0",
    2: "h2-eth1",
    -1: "h2-eth0",
}


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def clear_download_dir():
    if os.path.exists(tmp_dir):
        shutil.rmtree(tmp_dir)
    os.makedirs(tmp_dir)


def get_playback_buffer_size() -> int:
    return len(playback_buffer_map)


class DownloadStat(Structure):
    """
    typedef struct picoquic_download_stat {
        double time;
        double throughput;
        uint64_t rtt;
        uint64_t one_way_delay_avg;
        uint64_t bandwidth_estimate;
        uint64_t total_bytes_lost;
        uint64_t total_received;
        uint64_t data_received;
    }picoquic_download_stat;
    """

    _fields_ = [
        ("time", c_double),
        ("throughput", c_double),
        ("rtt", c_uint64),
        ("one_way_delay_avg", c_uint64),
        ("bandwidth_estimate", c_uint64),
        ("total_bytes_lost", c_uint64),
        ("total_received", c_uint64),
        ("data_received", c_uint64),
    ]


class Downloader:
    # define an init function with optional parameters and type hints
    def __init__(self,
                 host: str = default_host,
                 port: int = default_port,
                 mpd_url: str = default_mpd_url,
                 scheduler: str = "contextual_bandit",
                 algorithm: str = "",
                 ):
        self.host = host
        self.port = port
        self.manager = Manager()
        self.scheduler = scheduler
        self.algorithm = algorithm

        self.radius = None
        self.initial_explore_done = False
        self.scheduled = defaultdict(int)

        self.download_queue = []
        for i in range(nb_paths):
            self.download_queue.append(self.manager.Queue(maxsize=1))

        self.libplayer = CDLL("./libplayer.so")

        self.url = parse_mpd(mpd_url)

        self.init_resolution, self.init_bitrate_level, self.init_bitrate = get_initial_bitrate()

        print("downloader inited with host: %s, port: %d" % (self.host, self.port))
        print("initial bitrate level: %d, initial bitrate: %f" % (self.init_bitrate_level, self.init_bitrate))

        self.downloaded_segments = 0

    def main_loop(self):
        if self.scheduler == "contextual_bandit":
            scheduling_process = Process(target=self.contextual_bandit_scheduling)
        elif self.scheduler == "roundrobin":
            scheduling_process = Process(target=self.roundrobin_scheduling)
        elif self.scheduler == "minrtt":
            scheduling_process = Process(target=self.minrtt_scheduling)
        else:
            # default scheduler set to roundrobin
            scheduling_process = Process(target=self.roundrobin_scheduling)

        scheduling_process.start()

        process_list = []
        for path_id in range(nb_paths):
            download_process = Process(target=self.download, args=(path_id,))
            process_list.append(download_process)
            download_process.start()

        scheduling_process.join()
        for path_id in range(nb_paths):
            process_list[path_id].join()

    def download(self, path_id: int):
        while True:
            if self.download_queue[path_id].qsize() > 0:
                task = self.download_queue[path_id].get()
                if task["eos"] == 1:
                    break

                stat = DownloadStat()

                _playback_buffer_ratio_before = playback_buffer_frame_ratio.value
                _rebuffering_ratio_before = rebuffering_ratio.value

                with stdout_redirected():
                    self.libplayer.download_segment(c_str(self.host), c_int(self.port), c_str(task["url"]),
                                                    c_str(if_name_mapping[task["path_id"]]),
                                                    c_str(tmp_dir),
                                                    byref(stat))

                print("downloaded %s on path %s" % (task["url"], task["path_id"]))
                self.downloaded_segments += 1

                playback_buffer_map[task["seg_no"]] = task

                _playback_buffer_ratio = playback_buffer_frame_ratio.value
                _rebuffering_ratio = rebuffering_ratio.value
                _bitrate_level_ratio = task["bitrate"] * 1.0 / get_max_bitrate()

                history.append({
                    "arm": task["arm"],
                    "seg_no": task["seg_no"],
                    "resolution": task["resolution"],
                    "bitrate": task["bitrate"],
                    "bitrate_ratio": _bitrate_level_ratio,
                    "throughput": stat.throughput,
                    "rtt": stat.one_way_delay_avg / 1000.0 * 2,
                    "playback_buffer_ratio": _playback_buffer_ratio_before,
                    "rebuffering_ratio": _rebuffering_ratio_before,
                    "playback_buffer_ratio_after": _playback_buffer_ratio,
                    "rebuffering_ratio_after": _rebuffering_ratio,
                    "path_id": task["path_id"],
                    "initial_explore": task["initial_explore"],
                    "reward": -1 * _bitrate_level_ratio if _rebuffering_ratio - _rebuffering_ratio_before > 0 else _bitrate_level_ratio,
                })

                if self.scheduler == "contextual_bandit":
                    if self.initial_explore_done:
                        self.radius.partial_fit(decisions=pd.Series([task["arm"]]),
                                                rewards=pd.Series([history[-1]["reward"]]), contexts=task["context"])

                self.download_queue[path_id].task_done()

                if DEBUG_WAIT:
                    time.sleep(DEBUG_WAIT_SECONDS)

    def sequential_scheduling(self):
        for i in range(1, len(self.url[self.init_resolution][self.init_bitrate_level])):
            task = self.url[self.init_resolution][self.init_bitrate_level][i]
            task["seg_no"] = i
            task["resolution"] = self.init_resolution
            task["bitrate"] = self.init_bitrate
            task["path_id"] = -1
            task["eos"] = 0
            self.download_queue[-1].put(task)

        self.download_queue[-1].put({
            "eos": 1
        })

    def get_latest_bw_on_path(self, path_id: int):
        for i in range(len(history) - 1, -1, -1):
            if history[i]["path_id"] == path_id:
                return history[i]["throughput"]
        return 0

    """
    :return min_rtt_path_id (0, 1)
    """
    def get_minrtt_path_id(self):
        min_rtt = 999999
        min_rtt_path_id = 0
        for path_id in range(nb_paths):
            for i in range(len(history) - 1, -1, -1):
                if history[i]["path_id"] - 1 == path_id:
                    if history[i]["rtt"] < min_rtt:
                        min_rtt = history[i]["rtt"]
                        min_rtt_path_id = path_id
                    break
        return min_rtt_path_id

    def initial_explore(self):
        seg_no = 1
        for path_id in range(1, nb_paths + 1):
            for r, m in reversed(bitrate_mapping.items()):
                for k, v in reversed(m.items()):
                    task = self.url[r][k][seg_no]
                    task["seg_no"] = seg_no
                    task["resolution"] = r
                    task["bitrate"] = v
                    task["path_id"] = path_id
                    task["eos"] = 0
                    task["initial_explore"] = 1
                    task["arm"] = (path_id - 1) * get_nb_bitrates() + k
                    self.scheduled[seg_no] = 1
                    self.download_queue[path_id - 1].put(task)
                    self.download_queue[path_id - 1].join()
                    seg_no += 1

        print(f"{bcolors.RED}all initial explore tasks are put into queue {bcolors.ENDC}")

        for i in range(nb_paths):
            self.download_queue[i].join()

        for h in history:
            print(h)

    """
    get the highest resolution, bitrate and bitrate_level to the given bandwidth
    :return: resolution, bitrate, bitrate_level
    """
    def get_closest_resolution_and_bitrate_level(self, bw: float):
        r, b, bl = 360, 4481.84, 6
        for resolution in bitrate_mapping:
            for bitrate_level in bitrate_mapping[resolution]:
                if bitrate_mapping[resolution][bitrate_level] < bw * 1000:
                    r = resolution
                    b = bitrate_mapping[resolution][bitrate_level]
                    bl = bitrate_level
                    return r, b, bl
        return r, b, bl

    def roundrobin_scheduling(self):
        print("##roundrobin scheduling is selected##")
        i = 1
        last_path_id = 0
        while True:
            if DEBUG and i > DEBUG_SEGMENTS:
                playback_buffer_map[i] = {
                    "eos": 1,
                    "frame_count": 0,
                }
                for i in range(nb_paths):
                    task = dict()
                    task["path_id"] = i+1
                    task["eos"] = 1
                    task["initial_explore"] = 0
                    self.download_queue[i].put(task)
                break

            if playback_buffer_frame_ratio.value >= 1:
                continue

            if self.scheduled[i] == 1:
                continue

            # 0, 1
            path_id = last_path_id ^ 1
            last_path_id = path_id

            bw = self.get_latest_bw_on_path(path_id+1)

            resolution, bitrate, bitrate_level = self.get_closest_resolution_and_bitrate_level(bw)
            print("bw on path %d is %f, r: %d, b: %f, bl: %d" % (path_id+1, bw, resolution, bitrate, bitrate_level))

            seg_no = i

            task = self.url[resolution][bitrate_level][seg_no]
            task["seg_no"] = seg_no
            task["arm"] = -1
            task["resolution"] = resolution
            task["bitrate"] = bitrate
            task["path_id"] = path_id+1
            task["eos"] = 0
            task["initial_explore"] = 0
            self.scheduled[seg_no] = 1
            self.download_queue[path_id].put(task)

            i += 1

    def minrtt_scheduling(self):
        print("##minrtt scheduling is selected##")

        self.initial_explore()
        self.initial_explore_done = True

        i = 13
        while True:
            if DEBUG and i > DEBUG_SEGMENTS:
                playback_buffer_map[i] = {
                    "eos": 1,
                    "frame_count": 0,
                }
                for i in range(nb_paths):
                    task = dict()
                    task["path_id"] = i + 1
                    task["eos"] = 1
                    task["initial_explore"] = 0
                    self.download_queue[i].put(task)
                break

            if playback_buffer_frame_ratio.value >= 1:
                continue

            if self.scheduled[i] == 1:
                continue

            # 0, 1
            path_id = self.get_minrtt_path_id()
            bw = self.get_latest_bw_on_path(path_id + 1)

            resolution, bitrate, bitrate_level = self.get_closest_resolution_and_bitrate_level(bw)
            print("bw on path %d is %f, r: %d, b: %f, bl: %d" % (path_id + 1, bw, resolution, bitrate, bitrate_level))

            seg_no = i

            task = self.url[resolution][bitrate_level][seg_no]
            task["seg_no"] = seg_no
            task["arm"] = -1
            task["resolution"] = resolution
            task["bitrate"] = bitrate
            task["path_id"] = path_id + 1
            task["eos"] = 0
            task["initial_explore"] = 0
            self.scheduled[seg_no] = 1
            self.download_queue[path_id].put(task)

            i += 1

    def contextual_bandit_scheduling(self):
        def get_history_arm():
            _history_arm = []
            for x in history:
                path_id = x["path_id"]
                bitrate = x["bitrate"]
                resolution = x["resolution"]
                bitrate_level = get_bitrate_level(resolution, bitrate)
                nb_bitrates = get_nb_bitrates()
                _history_arm.append((path_id - 1) * nb_bitrates + bitrate_level)

            return _history_arm

        def get_history_rtt():
            _history_rtt = [x["rtt"] for x in history]
            return _history_rtt

        def get_latest_rtt():
            return history[len(history) - 1]["rtt"]

        def get_history_throughput():
            _history_throughput = [x["throughput"] for x in history]
            return _history_throughput

        def get_history_mean_throughput_on_path(path_id: int):
            _history_throughput = [x["throughput"] for x in history if x["path_id"] == path_id]
            return numpy.mean(_history_throughput)

        def get_history_mean_rtt_on_path(path_id: int):
            _history_rtt = [x["rtt"] for x in history if x["path_id"] == path_id]
            return numpy.mean(_history_rtt)

        def get_latest_throughput():
            return history[len(history) - 1]["throughput"]

        def get_history_bitrate():
            _history_bitrate = [x["bitrate"] for x in history]
            return _history_bitrate

        def get_latest_bitrate():
            return history[len(history) - 1]["bitrate"]

        def get_latest_bitrate_level():
            bitrate = history[len(history) - 1]["bitrate"]
            resolution = history[len(history) - 1]["resolution"]
            return get_bitrate_level(resolution, bitrate)

        def get_history_rebuffering_ratio():
            _history_rebuffering_ratio = [x["rebuffering_ratio"] for x in history]
            return _history_rebuffering_ratio

        def get_latest_rebuffering_ratio():
            return rebuffering_ratio.value

        def get_history_playback_buffer_ratio():
            _history_playback_buffer_ratio = [x["playback_buffer_ratio"] for x in history]
            return _history_playback_buffer_ratio

        def get_latest_playback_buffer_ratio():
            return playback_buffer_frame_ratio.value
            # return get_playback_buffer_size() * 1.0 / max_playback_buffer_size

        def get_history_reward():
            _history_reward = [x["reward"] for x in history]
            return _history_reward

        def parse_predication(predication) -> (int, int, float, int):
            path_id = (predication - 1) // get_nb_bitrates() + 1
            bitrate_level = (predication - 1) % get_nb_bitrates() + 1

            # get resolution from bitrate_level in bitrate_mapping
            resolution = get_resolution(bitrate_level)
            return path_id, resolution, bitrate_mapping[resolution][bitrate_level], bitrate_level

        def get_smallest_unscheduled_segment():
            for i in range(1, nb_segments):
                if self.scheduled[i] == 0:
                    return i

        self.initial_explore()

        self.initial_explore_done = True

        history_arm = get_history_arm()
        history_reward = get_history_reward()[:len(history_arm)]

        train_df = pd.DataFrame({
            "arm": history_arm,
            "bitrate": get_history_bitrate(),
            "throughput": get_history_throughput(),
            "rtt": get_history_rtt(),
            "playback_buffer_ratio": get_history_playback_buffer_ratio()[:len(history_arm)],
            "reward": history_reward,
        })

        scaler = StandardScaler()
        train = scaler.fit_transform(train_df[["throughput", "rtt", "playback_buffer_ratio"]].values.astype('float64'))

        if self.algorithm == "LinUCB":
            self.radius = MAB(
                arms=build_arms(),
                learning_policy=LearningPolicy.LinUCB(alpha=config.linucb_alpha),
            )
            print("Initialized MAB with LinUCB and alpha = ", config.linucb_alpha)
        elif self.algorithm == "LinTS":
            self.radius = MAB(
                arms=build_arms(),
                learning_policy=LearningPolicy.LinTS(alpha=config.lints_alpha),
            )
            print("Initialized MAB with LinTS and alpha = ", config.lints_alpha)
        elif self.algorithm == "LinGreedy":
            self.radius = MAB(
                arms=build_arms(),
                learning_policy=LearningPolicy.LinGreedy(epsilon=config.egreedy_epsilon),
            )
            print("Initialized MAB with LinGreedy and epsilon = ", config.egreedy_epsilon)
        else:
            pass

        self.radius.fit(decisions=train_df["arm"], rewards=train_df["reward"], contexts=train)

        nb_segments = len(self.url[self.init_resolution][self.init_bitrate_level])
        latest_selected_arm = 1

        while True:
            i = get_smallest_unscheduled_segment()
            if i > nb_segments:
                task1 = dict()
                task1["path_id"] = 1
                task1["eos"] = 1
                task1["initial_explore"] = 0
                task2 = dict()
                task2["path_id"] = 2
                task2["eos"] = 1
                task2["initial_explore"] = 0
                self.download_queue[0].put(task1)
                self.download_queue[1].put(task2)

                break

            if DEBUG and i > DEBUG_SEGMENTS:
                playback_buffer_map[i] = {
                    "eos": 1,
                    "frame_count": 0,
                }
                task1 = dict()
                task1["path_id"] = 1
                task1["eos"] = 1
                task1["initial_explore"] = 0
                task2 = dict()
                task2["path_id"] = 2
                task2["eos"] = 1
                task2["initial_explore"] = 0
                self.download_queue[0].put(task1)
                self.download_queue[1].put(task2)
                break

            if playback_buffer_frame_ratio.value >= 1:
                continue

            if self.scheduled[i] == 1:
                continue

            for k in range(2):
                if self.download_queue[k].qsize() == 0:
                    test_df = pd.DataFrame({
                        "throughput": [get_history_mean_throughput_on_path(k + 1)],
                        "rtt": [get_history_mean_rtt_on_path(k + 1)],
                        "playback_buffer_ratio": [get_latest_playback_buffer_ratio()],
                    })

                    test = scaler.transform(test_df.values.astype('float64'))
                    if k == 0:
                        prediction = self.radius.predict(test)
                        # partial_fit is done after download finished, because only then we know the reward
                        latest_selected_arm = prediction
                        seg_no = i
                        print("path ", k + 1, " is empty ", " raw prediction: ", prediction, " seg_no: ", i)
                    else:
                        prediction = latest_selected_arm
                        seg_no = i + 5
                        print("path ", k + 1, " is empty ", " raw prediction: ", prediction, " seg_no: ", i+5)

                    path_id, resolution, bitrate, bitrate_level = parse_predication(prediction)
                    raw_prediction = prediction
                    path_id = k+1
                    task = self.url[resolution][bitrate_level][seg_no]
                    task["seg_no"] = seg_no
                    task["resolution"] = resolution
                    task["bitrate"] = bitrate
                    task["path_id"] = path_id
                    task["eos"] = 0
                    task["arm"] = prediction
                    task["context"] = test
                    task["initial_explore"] = 0
                    self.scheduled[seg_no] = 1
                    self.download_queue[path_id - 1].put(task)
                    time.sleep(0.1)
                    print(
                        f"{bcolors.RED}{bcolors.BOLD}seg_no: %d, {bcolors.WARNING}raw predication: %d, selected arm: "
                        f"%d, path_id: %d,"
                        f"resolution: %d, bitrate_level: %d{bcolors.ENDC}" % (
                            seg_no, raw_prediction, prediction, path_id, resolution,
                            bitrate_level))
