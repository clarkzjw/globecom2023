//
// Created by clarkzjw on 1/5/23.
//

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <tplayer.h>
#include <vector>
#include <string>
#include <regex>
#include <functional>
#include <shared_mutex>

using namespace std;

extern double initial_bitrate;
//extern int initial_resolution;
extern int nb_segments;

#define INIT_BITRATE_LEVEL 1

extern vector<SegmentPlaybackInfo> playback_info_vec;
int first_segment_downloaded = 0;
int bitrate_level = INIT_BITRATE_LEVEL;
extern int cur_resolution;
extern double cur_bitrate;
extern TicToc buffering_timer;
extern std::chrono::system_clock::time_point player_start;




map<int, RewardFactor> tmp_reward_map;

extern map<int, map<int, double>> bitrate_mapping;
extern vector<double> path_rtt[nb_paths];
extern vector<struct reward_item> history_rewards;




class PathSelector {
private:
    std::mutex path_mutexes[nb_paths];
//    std::map<int, BS::thread_pool*> path_pool;
    mutable std::shared_mutex path_mutex;
    int previous_path_id;

public:
    BS::thread_pool* path_pool[nb_paths]{};

    // TODO
    map<Algorithm, CallbackDownload> algorithm_map;

    PathSelector() {
        for (int i = 0; i < nb_paths; i++) {
            auto *p = new BS::thread_pool(1);
            path_pool[i] = p;
        }
        previous_path_id = 1;

        printf("PathSelected inited\n");
    }

    ~PathSelector() {
        for (int i = 0; i < nb_paths; i++) {
            path_pool[i]->wait_for_tasks();
        }
    }


    /*
     * Pseudo Round Robin Scheduling
     * the task is scheduled to the path which has the lowest index and is also available
     * */
    void pseudo_roundrobin_scheduler(const struct DownloadTask& t, const CallbackDownload& download_f)
    {
        int done = 0;
        while (true) {
            for (int path_id = 0; path_id < nb_paths; path_id++) {
                if (path_mutexes[path_id].try_lock()) {
                    path_pool[path_id]->push_task(download_f, path_id, t, &path_mutexes[path_id]);
                    done = 1;
                    break;
                }
            }
            if (done) {
                break;
            }
        }
    }

    void roundrobin_scheduler(const struct DownloadTask& t, const CallbackDownload& download_f)
    {
        std::unique_lock p_lock(path_mutex);
        // reverse previous_path_id
        // i.e., 0->1, 1->0
        // only valid for two paths
        previous_path_id = previous_path_id ^ 1;

        printf("=====segment %d assigned to path %d\n", t.seg_no, previous_path_id);
        path_pool[previous_path_id]->push_task(download_f, previous_path_id, t, nullptr);
    }


    /*
     * Min RTT Scheduling
     * 1. If there's an empty path available, choose it regardless of the RTT
     * 2. If all paths are occupied, choose one of them based on previous RTT measurement
     * */
    void minrtt_scheduler(const struct DownloadTask& t, const CallbackDownload& download_f) {

        auto get_minrtt_path_id = [this]() {
            double minrtt = INT_MAX;
            int path_id = 0;
            for (int i = 0; i < nb_paths; i++) {
                // if there's an empty path available, return it regardless the rtt
                if (path_pool[i]->get_tasks_total() == 0) {
                    return i;
                }

                if (!path_rtt[i].empty()) {
                    double v = path_rtt[i][path_rtt[i].size() - 1];
                    if (v < minrtt) {
                        minrtt = v;
                        path_id = i;
                    }
                } else if (path_rtt[i].empty() == true && path_pool[i]->get_tasks_total() == 0) {
                    // if path_rtt[i] is empty, which means it hasn't been selected and tried before
                    // so use it for exploration
                    return i;
                } else {
                    // there's no RTT measurement on this path yet
                    // but there's already tasks assigned to this path but not finished yet
                    // re-decide after N seconds
                    return -1;
                }
            }
            return path_id;
        };

        int path_id;
        while (true) {
            path_id = get_minrtt_path_id();
            if (path_id == -1) {
                PortableSleep(0.1);
                continue;
            } else {
                break;
            }
        }

        printf("=====segment %d assigned to path %d\n", t.seg_no, path_id);
        path_pool[path_id]->push_task(download_f, path_id, t, nullptr);
    }

};

/*
 * Downloader
 * */
void download(int path_id, const struct DownloadTask& t, std::mutex *path_mutex)
{
    string if_name;
    if (path_id == 0) {
        if_name = path_ifname_vec[0];
    } else if (path_id == 1) {
        if_name = path_ifname_vec[1];
    } else {
        if_name = "";
    }

    printf("sequential download, interface = %s\n", if_name.c_str());
    string filename = urls[bitrate_level-1][t.seg_no].url;
    printf("%s\n", filename.c_str());

    PerSegmentStats pst;
    // if this is new
    seg_stats[t.seg_no].path_id = path_id;
    seg_stats[t.seg_no].resolution = t.resolution;

    struct picoquic_download_stat picoquic_st { };
    TicToc timer;

    auto *config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
    picoquic_config_init(config);
    config->out_dir = "./tmp";
    config->sni = "test";

    pst.start = timer.tic();
    int ret = quic_client(host.c_str(), port, config, 0, 0, filename.c_str(), if_name.c_str(), &picoquic_st);
    pst.end = timer.tic();

    printf("download ret = %d\n", ret);

    free(config);

    if (t.seg_no == 1) {
        first_segment_downloaded = 1;
    }

    seg_stats[t.seg_no].bitrate = cur_bitrate;
    seg_stats[t.seg_no].path_id = path_id;
    seg_stats[t.seg_no].cur_rtt = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
    seg_stats[t.seg_no].bandwidth_estimate = picoquic_st.bandwidth_estimate;
    seg_stats[t.seg_no].start = pst.start;
    seg_stats[t.seg_no].end = pst.end;
    seg_stats[t.seg_no].download_speed = picoquic_st.throughput;

    path_rtt[path_id].push_back(seg_stats[t.seg_no].cur_rtt);
    pst.cur_rtt = seg_stats[t.seg_no].cur_rtt;

    // adjust bitrate
    double buffering_ratio = buffering_event_count_ratio_on_path(path_id);
//    double buffering_ratio = previous_buffering_time_on_path(path_id) / previous_total_buffering_time();
    double previous_avg_rtt = get_previous_average_rtt(path_id, 2);
    double latest_avg_rtt = get_latest_average_rtt();

    int alpha = 1;
    int beta = 1;
    int gamma = 1;

    double reward = alpha * (1.0 / buffering_ratio) + beta * (latest_avg_rtt / pst.cur_rtt) + gamma * (cur_bitrate / get_maximal_bitrate());

    tmp_reward_map[t.seg_no] = {buffering_ratio,
                                previous_avg_rtt,
                                latest_avg_rtt,
                                bitrate_mapping[t.resolution][t.bitrate_level],
                                pst.cur_rtt,
                                reward,
                                path_id,
                                t.seg_no};

    seg_stats[t.seg_no].latest_avg_rtt = latest_avg_rtt;
    seg_stats[t.seg_no].average_reward_so_far = get_previous_most_recent_average_reward(5);
    seg_stats[t.seg_no].reward = reward;

    history_rewards.push_back({reward, path_id});

    PlayableSegment ps {};
    ps.nb_frames = 48;
    ps.eos = 0;
    ps.seg_no = t.seg_no;
    ps.duration_seconds = urls[t.bitrate_level][t.seg_no].duration_seconds;
    player_buffer_map[t.seg_no] = ps;
    player_buffer.push(ps);

    if (path_mutex != nullptr) {
        path_mutex->unlock();
    }
}

extern string alg;



void main_downloader() {
    int i = 1;

    double bitrate = get_bitrate_from_bitrate_level(bitrate_level);
    cur_resolution = get_resolution_by_bitrate(bitrate);

    PathSelector path_selector;

    // TODO
    // download init.mp4

    struct DownloadTask first_seg;
    first_seg.filename = urls[bitrate_level-1][1].url;
    first_seg.seg_no = 1;
    first_seg.bitrate_level = bitrate_level;
    first_seg.resolution = cur_resolution;

//    path_selector.pseudo_roundrobin_scheduler(first_seg, download);
//    path_selector.roundrobin_scheduler(first_seg, download);
    path_selector.minrtt_scheduler(first_seg, download);

    auto check_download_buffer_full = [](PathSelector& path_selector) {
        int total = 0;
        for (int i = 0; i < nb_paths; i++) {
            total += path_selector.path_pool[i]->get_tasks_total();
        }
        if (total > nb_paths) {
            return true;
        }
        return false;
    };

    while (true) {
        if (first_segment_downloaded == 1) {
            break;
        }
        PortableSleep(0.01);
    }

    i++;

    while (true) {
        if (i >= nb_segments) {
            break;
        }

        if (player_buffer.size() >= PLAYER_BUFFER_MAX_SEGMENTS || check_download_buffer_full(path_selector)) {
            PortableSleep(0.01);
            continue;
        }

        string filename = urls[bitrate_level-1][i].url;
        printf("%s\n", filename.c_str());

        struct DownloadTask t;
        t.filename = filename;
        t.seg_no = i;
        t.resolution = cur_resolution;
        t.bitrate_level = bitrate_level;

//        path_selector.pseudo_roundrobin_scheduler(t, download);
//        path_selector.roundrobin_scheduler(t, download);
        path_selector.minrtt_scheduler(t, download);
        i++;
    }

    struct PlayableSegment ps;
    ps.eos = 1;
    player_buffer.push(ps);
    player_buffer_map[nb_segments] = ps;
}


void print_segment_download_stats() {
    printf("\nper segment download metrics\n");
    for (auto& [seg_no, value] : seg_stats) {
        value.download_time = double(std::chrono::duration_cast<std::chrono::microseconds>(value.end - value.start).count()) / 1e6;

        std::cout << seg_no << "="
                  << " used: " << value.download_time << " seconds,"
                  << " starts at " << epoch_to_relative_seconds(playback_start, value.start)
                  << " ends at " << epoch_to_relative_seconds(playback_start, value.end)
                  << " speed: " << value.download_speed << " Mbps"
                  << " bitrate: " << value.bitrate
                  << " resolution: " << value.resolution
                  << " path_id: " << value.path_id
                  << " previous avg reward: " << value.average_reward_so_far
                  << " reward: " << value.reward
                  << " rtt: " << value.cur_rtt
                  << endl;
    }
}

void print_playback_info() {
    printf("\nplayback_info_vec event metrics, playback event count: %zu\n", playback_info_vec.size());
    for (auto& spi : playback_info_vec) {
        std::cout << "playback event, seg_no: " << spi.seg_no << ", start: " << spi.playback_start_second << ", end: "
                  << spi.playback_end_second << ", duration: " << spi.playback_duration << endl;
    }
}

void print_tmp_reward_info() {
    printf("\ntmp_reward_info_vec event metrics, tmp reward event count: %zu\n", tmp_reward_map.size());

    for (int i = 0; i < nb_paths; i++) {
        for (auto& tri : tmp_reward_map) {
            if (tri.second.path_id == i) {
                cout << "seg_no: " << tri.second.seg_no << \
                        " buffering ratio: " << tri.second.buffering_ratio << \
                        " path_id: " << tri.second.path_id << \
                        " previous avg rtt on path: " << tri.second.previous_avg_rtt << \
                        " latest avg rtt: " << tri.second.latest_avg_rtt << \
                        " bitrate: " << tri.second.bitrate << \
                        " rtt: " << tri.second.rtt << \
                        " reward: " << tri.second.reward << endl;
            }
        }
        printf("\n\n");
    }
}

[[noreturn]] void check_player_buffer() {
    while (true) {
        printf("player buffer: %zu\n", player_buffer.size());
        PortableSleep(5);
    }
}


void start()
{
    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(main_downloader);
    std::thread thread_playback(main_player_mock);

    thread_download.join();
    thread_playback.join();

    print_segment_download_stats();

    print_playback_info();

    print_tmp_reward_info();

    save_metrics_to_file();
}

