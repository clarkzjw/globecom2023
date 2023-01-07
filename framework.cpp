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


struct RewardFactor {
    double buffering_ratio;
    double previous_avg_rtt;
    double bitrate;
    double rtt;
    double reward;
    int seg_no;
};

vector<RewardFactor> tmp_reward_vec;

int adjust_bitrate(int seg_no, int path_id, double rtt, double bitrate) {
    double buffering_ratio = previous_buffering_time_on_path(path_id) / epoch_to_relative_seconds(player_start, Tic());
    double previous_avg_rtt = get_previous_average_rtt(path_id, 2);

    int alpha = 1;
    int beta = 1;

    double reward = alpha * (1 - buffering_ratio) + beta * (1 - rtt / previous_avg_rtt);
    tmp_reward_vec.push_back({buffering_ratio, previous_avg_rtt, bitrate, rtt, reward, seg_no});

    return 0;
}

extern map<int, map<int, double>> bitrate_mapping;
extern vector<double> path_rtt[nb_paths];
extern vector<struct reward_item> history_rewards;

/*
 * Sequential Download
 * */
void sequential_download(int path_id, const struct DownloadTask& t)
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
    if (seg_stats.find(t.seg_no) != seg_stats.end()) {
        pst.path_id = path_id;
        pst.resolution = t.resolution;
        seg_stats.insert({ t.seg_no, pst });
    }

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

    seg_stats[t.seg_no].cur_rtt = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
    seg_stats[t.seg_no].bandwidth_estimate = picoquic_st.bandwidth_estimate;
    seg_stats[t.seg_no].start = pst.start;
    seg_stats[t.seg_no].end = pst.end;
    seg_stats[t.seg_no].download_speed = picoquic_st.throughput;

    path_rtt[path_id].push_back(seg_stats[t.seg_no].cur_rtt);
    pst.cur_rtt = seg_stats[t.seg_no].cur_rtt;

    // adjust bitrate
    double buffering_ratio = previous_buffering_time_on_path(path_id) / epoch_to_relative_seconds(player_start, Tic());
    double previous_avg_rtt = get_previous_average_rtt(path_id, 2);

    int alpha = 1;
    int beta = 1;

    double reward = alpha * (1 - buffering_ratio) + beta * (1 - pst.cur_rtt / previous_avg_rtt);
    tmp_reward_vec.push_back({buffering_ratio, previous_avg_rtt, bitrate_mapping[t.resolution][t.bitrate_level], pst.cur_rtt, reward, t.seg_no});

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
}

void main_downloader() {
    BS::thread_pool download_thread_pool(nb_paths);

    int i = 1;

    double bitrate = get_bitrate_from_bitrate_level(bitrate_level);
    cur_resolution = get_resolution_by_bitrate(bitrate);

    struct DownloadTask first_seg;
    first_seg.filename = urls[bitrate_level-1][1].url;
    first_seg.seg_no = 1;
    first_seg.bitrate_level = bitrate_level;
    first_seg.resolution = cur_resolution;
    int path_id = 0;

    download_thread_pool.push_task(sequential_download, path_id, first_seg);

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

        if (player_buffer.size() >= PLAYER_BUFFER_MAX_SEGMENTS || download_thread_pool.get_tasks_total() >= PLAYER_BUFFER_MAX_SEGMENTS) {
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

        download_thread_pool.push_task(sequential_download, path_id, t);
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
//                  << " gamma: " << value.gamma
                  << " reward: " << value.reward
//                  << " gamma_throughput: " << value.gamma_throughput
//                  << " gamma_avg_throughput: " << value.gamma_avg_throughput
                  << " rtt: " << value.cur_rtt
//                  << " gamma_avg_rtt: " << value.gamma_avg_rtt
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
    printf("\ntmp_reward_info_vec event metrics, tmp reward event count: %zu\n", tmp_reward_vec.size());
    for (auto& tri : tmp_reward_vec) {
        cout << "seg_no: " << tri.seg_no << \
        " buffering ratio: " << tri.buffering_ratio << \
        " previous avg rtt: " << tri.previous_avg_rtt << \
        " bitrate: " << tri.bitrate << \
        " rtt: " << tri.rtt << \
        " reward: " << tri.reward << endl;
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
}

