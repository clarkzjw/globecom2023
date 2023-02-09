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

extern vector<struct reward_item> history_rewards;

extern std::vector<BufferEvent> buffer_events_vec;

/*
 * Downloader
 * */
void download(int path_id, const struct DownloadTask& t, std::mutex *path_mutex, set_reward_callback* func)
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
    printf("+=+=+=+=+=+= first seg_stats inserted at %ld\n", buffer_events_vec[0].start.time_since_epoch().count());
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
    double max_rtt = get_max_rtt();

    double alpha = 1;
    double beta = 1;
    double gamma = 1;

    double reward = 0;
    if (buffering_ratio != 0) {
        reward = alpha * (max_rtt / pst.cur_rtt)  + beta * (buffering_ratio * buffering_ratio) + gamma * (cur_bitrate / get_maximal_bitrate());
    } else {
        reward = alpha * (max_rtt / pst.cur_rtt)  + beta * (buffering_ratio * buffering_ratio) + gamma * (cur_bitrate / get_maximal_bitrate());
    }

    tmp_reward_map[t.seg_no] = {buffering_ratio,
                                previous_avg_rtt,
                                latest_avg_rtt,
                                bitrate_mapping[t.resolution][t.bitrate_level],
                                pst.cur_rtt,
                                reward,
                                path_id,
                                t.seg_no};

    seg_stats[t.seg_no].max_rtt = max_rtt;
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
    if (func) {
        printf("setting reward for mab simulator, %f on path %d\n", reward, path_id);
        (*func)(reward, path_id);
    }
}

extern string alg;
extern Algorithm scheduling_algorithm;
extern std::thread *thread_playback;

void initial_explore() {
    struct BufferEvent be { };
    be.start = player_start;
    be.path_id = 1;
    buffer_events_vec.push_back(be);
    thread_playback = new std::thread(main_player_mock);

    BS::thread_pool initial(nb_paths);
    for (int i = 0; i < nb_paths; i++) {
        struct DownloadTask seg;
        seg.filename = urls[bitrate_level-1][i+1].url;
        seg.seg_no = i+1;
        seg.bitrate_level = bitrate_level;
        seg.resolution = cur_resolution;

        int path_id = i;
        initial.push_task(download, path_id, seg, nullptr, nullptr);
    }
    initial.wait_for_tasks();
}

void main_downloader() {

    auto check_download_buffer_full = [](PathSelector& path_selector) {
        int total = 0;
        for (int i = 0; i < nb_paths; i++) {
            total += (int)path_selector.path_pool[i]->get_tasks_total();
        }
        if (total > nb_paths) {
            return true;
        }
        return false;
    };


    int i = 1;

    double bitrate = get_bitrate_from_bitrate_level(bitrate_level);
    cur_resolution = get_resolution_by_bitrate(bitrate);

    PathSelector path_selector;

    // TODO
    // download init.mp4

    player_start = Tic();

    if (scheduling_algorithm != Algorithm::mab) {
        struct DownloadTask first_seg;
        first_seg.filename = urls[bitrate_level-1][1].url;
        first_seg.seg_no = 1;
        first_seg.bitrate_level = bitrate_level;
        first_seg.resolution = cur_resolution;

//        path_selector.pseudo_roundrobin_scheduler(first_seg, download);
//        path_selector.roundrobin_scheduler(first_seg, download);
        path_selector.minrtt_scheduler(first_seg, download);

        while (true) {
            if (first_segment_downloaded == 1) {
                break;
            }
            PortableSleep(0.01);
        }

        i++;
    } else {
//        initial_explore();
//        i += nb_paths;
    }

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
//        path_selector.minrtt_scheduler(t, download);
//        path_selector.mab_scheduler(t, download);
        path_selector.linucb_scheduler(t, download);

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
                  << " max_rtt: " << value.max_rtt
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

std::thread *thread_playback;

void start()
{
    TicToc global_timer;
    playback_start = global_timer.tic();

    if (scheduling_algorithm == Algorithm::mab) {
        std::thread thread_download(main_downloader);

        thread_download.join();
        (*thread_playback).join();
    } else {
        std::thread thread_download(main_downloader);
        thread_playback = new std::thread(main_player_mock);

        thread_download.join();
        (*thread_playback).join();
    }


    print_segment_download_stats();

    print_playback_info();

    print_tmp_reward_info();

    save_metrics_to_file();
}

