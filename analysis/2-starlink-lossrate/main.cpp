//
// Created by clarkzjw on 1/3/23.
//

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include "tplayer.h"
#include <vector>
#include <string>
#include <regex>
#include <numeric>

using namespace std;

string host = "34.168.115.179";
int port = 4433;

std::vector<DownloadStats> stats;
std::map<int, PerSegmentStats> seg_stats;
std::queue<PlayableSegment> player_buffer;
std::map<int, PlayableSegment> player_buffer_map;
std::queue<DownloadTask> tasks;
tic_clock::time_point playback_start;
picoquic_quic_config_t* quic_config = nullptr;
dash::mpd::IMPD* mpd_file;
vector<vector<struct SegmentInfo>> urls;

//char* path_name[2] = { "h2-eth0", "wlan0" };
vector<string> path_ifname_vec = { "h2-eth0", "wlan0" };

/*
 * Sequential Download
 * */
int nb_segments = 2;
int nb_rounds = 1;

std::vector<double> v_delay;
std::vector<double> v_throughput;

void sequential_download()
{
    int path_id = 1;
    printf("sequential download\n");

    int count = 0;
    string filename = string("/360.zip");
    printf("%s\n", filename.c_str());

    quic_config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
    picoquic_config_init(quic_config);
    quic_config->out_dir = "./tmp";
    quic_config->sni = "test";

    struct picoquic_download_stat picoquic_st { };
    string if_name = path_ifname_vec[path_id];

    int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), if_name.c_str(), &picoquic_st);
    printf("download ret = %d\n", ret);

    // this is rtt
    double delay = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
    printf("estimated delay: %f\n", delay);

    double throughput = picoquic_st.throughput;
    cout << delay << ", " << throughput << "\n";

    free(quic_config);
    count++;

    v_delay.push_back(delay);
    v_throughput.push_back(throughput);
}


int main(int argc, char* argv[])
{
    sequential_download();

    return 0;
}
