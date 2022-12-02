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

char* path_name[2] = { "h2-eth0", "h2-eth1" };
int PLAYER_BUFFER_MAX_SEGMENTS = 5;

int enable_multipath = 1;
char const* multipath_links = "10.0.2.2/2,10.0.3.2/3";
char const* multipath_fast_link = "10.0.2.2/2";
char const* multipath_slow_link = "10.0.3.2/3";

string local_mpd_url = "./BBB-I-1080p.mpd";

auto level = 3; // 0, 1, 2, 3

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

/*
 * Sequential Download
 * */
void sequential_download()
{
    printf("sequential download\n");
    for (int i = 1; i < 10; i++) {
        string filename = string("/1080/").append(urls[0][i]);
        printf("%s\n", filename.c_str());

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "", NULL);
        printf("download ret = %d\n", ret);
    }
}

int nb_segments = 30;

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

    quic_config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));

    picoquic_config_init(quic_config);
    quic_config->out_dir = "./tmp";

//    sequential_download();
//    multipath_picoquic_minRTT();
//    multipath_round_robin();
    multipath_mab();

    return 0;
}
