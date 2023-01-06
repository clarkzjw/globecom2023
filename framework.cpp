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

extern int initial_bitrate;
extern int initial_resolution;
extern int nb_segments;

extern vector<SegmentPlaybackInfo> playback_info_vec;
int first_segment_downloaded = 0;

/*
 * Sequential Download
 * */
void sequential_download(int path_id, struct DownloadTask t)
{
    string if_name;
    if (path_id == 0) {
        if_name = path_ifname_vec[0];
    } else if (path_id == 1) {
        if_name = path_ifname_vec[1];
    } else {
        if_name = "";
    }

    int cur_resolution = 1080;

    printf("sequential download, interface = %s\n", if_name.c_str());
    string filename = urls[0][t.seg_no].url;
    printf("%s\n", filename.c_str());

    struct picoquic_download_stat picoquic_st { };
    PerSegmentStats pst;
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

    if (!seg_stats.contains(t.seg_no)) {
        pst.path_id = path_id;

        pst.one_way_delay_avg = picoquic_st.one_way_delay_avg;
        pst.rtt_delay_estimate = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
        pst.download_speed = picoquic_st.throughput;

        pst.bandwidth_estimate = picoquic_st.bandwidth_estimate;
        pst.rtt = picoquic_st.rtt;
        pst.total_received = picoquic_st.total_received;
        pst.total_bytes_lost = picoquic_st.total_bytes_lost;
        pst.data_received = picoquic_st.data_received;
        pst.resolution = cur_resolution;


        pst.gamma_throughput = picoquic_st.throughput;
        pst.gamma_avg_throughput = get_previous_average_throughput(path_id);


        pst.gamma_rtt = pst.rtt_delay_estimate;
        pst.gamma_avg_rtt = get_previous_average_rtt(path_id);

        PlayableSegment ps {};
        ps.nb_frames = 48;
        ps.eos = 0;
        ps.seg_no = t.seg_no;
        ps.duration_seconds = urls[t.bitrate_level][t.seg_no].duration_seconds;
        player_buffer_map[t.seg_no] = ps;
        player_buffer.push(ps);

        seg_stats.insert({ t.seg_no, pst });
    }
}

int bitrate_level = 1;


void main_downloader() {
    BS::thread_pool download_thread_pool(nb_paths);

    int i = 1;

    int cur_bitrate = initial_bitrate;
    int cur_resolution = initial_resolution;

    struct DownloadTask first_seg;
    first_seg.filename = urls[bitrate_level][1].url;
    first_seg.seg_no = 1;
    first_seg.bitrate_level = bitrate_level;
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

        string filename = urls[bitrate_level][i].url;
        printf("%s\n", filename.c_str());

        struct DownloadTask t;
        t.filename = filename;
        t.seg_no = i;
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
                  << " gamma: " << value.gamma
                  << " reward: " << value.reward
                  << " gamma_throughput: " << value.gamma_throughput
                  << " gamma_avg_throughput: " << value.gamma_avg_throughput
                  << " gamma_rtt: " << value.gamma_rtt
                  << " gamma_avg_rtt: " << value.gamma_avg_rtt
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
}

