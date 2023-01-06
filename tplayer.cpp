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
vector<vector<struct SegmentInfo>> urls;

vector<string> path_ifname_vec{"h2-eth0", "h2-eth1"};

int PLAYER_BUFFER_MAX_SEGMENTS = 5;

char const* multipath_links = "10.0.2.2/2,10.0.3.2/3";

string local_mpd_url = "../dataset/BigBuckBunny/mpd/stream.mpd";

auto level = 3; // 0, 1, 2, 3

// max available segments: 299
int nb_segments = 10;

extern int initial_bitrate;
extern int initial_resolution;

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
    mpd_file = parse_mpd(local_mpd_url);
    urls = get_segment_urls(mpd_file);

    // start download and playback
    start();

    return 0;
}
