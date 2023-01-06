//
// Created by Jinwei Zhao on 2022-08-12.
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

string host;
int port;

std::vector<DownloadStats> stats;
std::map<int, PerSegmentStats> seg_stats;
std::queue<PlayableSegment> player_buffer;
std::map<int, PlayableSegment> player_buffer_map;
std::queue<DownloadTask> tasks;
tic_clock::time_point playback_start;
picoquic_quic_config_t* quic_config = nullptr;
dash::mpd::IMPD* mpd_file;
vector<vector<string>> urls;

vector<string> path_ifname_vec{"h2-eth0", "h2-eth1"};

int PLAYER_BUFFER_MAX_SEGMENTS = 5;

int enable_multipath = 1;
char const* multipath_links = "10.0.2.2/2,10.0.3.2/3";
char const* multipath_fast_link = "10.0.2.2/2";
char const* multipath_slow_link = "10.0.3.2/3";

string local_mpd_url = "./videos/BigBuckBunny/mpd/stream.mpd";

auto level = 3; // 0, 1, 2, 3

// max number of segments = 139
int nb_segments = 10;

double epoch_to_relative_seconds(tic_clock::time_point start, tic_clock::time_point end)
{
    return double(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1e6;
}

int get_filesize(const string& req_filename)
{
    std::string default_dir = "./tmp/";
    std::string a = req_filename.substr(1);
    std::replace(a.begin(), a.end(), '/', '_');
    std::string path = default_dir + a;

    std::filesystem::path p { path };
    size_t realsize = std::filesystem::file_size(p);
    std::cout << "filesize: " << realsize << std::endl;
    return (int)realsize;
}


void print_segment_stats() {
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
    string filename = urls[0][t.seg_no];
    printf("%s\n", filename.c_str());

    struct picoquic_download_stat picoquic_st { };
    PerSegmentStats pst;
    TicToc timer;

    picoquic_quic_config_t *config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
    picoquic_config_init(config);
    config->out_dir = "./tmp";
    config->sni = "test";

    pst.start = timer.tic();
    int ret = quic_client(host.c_str(), port, config, 0, 0, filename.c_str(), if_name.c_str(), &picoquic_st);
    pst.end = timer.tic();

    printf("download ret = %d\n", ret);

    free(config);

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
        player_buffer_map[t.seg_no] = ps;
        player_buffer.push(ps);

        seg_stats.insert({ t.seg_no, pst });
    }
}

extern int initial_bitrate;
extern int initial_resolution;

void _downloader() {
    BS::thread_pool download_thread_pool(nb_paths);

    int i = 1;

    int cur_bitrate = initial_bitrate;
    int cur_resolution = initial_resolution;
    int bitrate_level = 1;

    while (true) {
        if (i >= nb_segments) {
            break;
        }

        if (player_buffer.size() >= PLAYER_BUFFER_MAX_SEGMENTS || download_thread_pool.get_tasks_total() >= PLAYER_BUFFER_MAX_SEGMENTS) {
            PortableSleep(0.01);
            continue;
        }

        string filename;
        string tmp = urls[bitrate_level][i];
        printf("%s\n", filename.c_str());


        struct DownloadTask t;
        t.filename = filename;
        t.seg_no = i;
        int path_id = 0;

        download_thread_pool.push_task(sequential_download, path_id, t);
        i++;
    }

    struct PlayableSegment ps;
    ps.eos = 1;
    player_buffer.push(ps);
    player_buffer_map[nb_segments] = ps;
}


void _main()
{
    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(_downloader);
    std::thread thread_playback(mock_player_hash_map);

    thread_download.join();
    thread_playback.join();

    print_segment_stats();
}


int main(int argc, char* argv[])
{
    // parse host and port
    if (argc < 3) {
        printf("%s host port\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = (int)strtol(argv[2], nullptr, 10);
    printf("host: %s, port: %d\n", host.c_str(), port);

    // read mpd file
    mpd_file = parse_mpd(local_mpd_url, quic_config);
    urls = get_segment_urls(mpd_file);

//    quic_config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
//
//    picoquic_config_init(quic_config);
//    quic_config->out_dir = "./tmp";

    _main();

    return 0;
}
