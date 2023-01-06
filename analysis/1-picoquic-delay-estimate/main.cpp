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

string host = "server.local.me";
int port = 4433;

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

string local_mpd_url = "./big.mpd";
//string local_mpd_url = "./BBB-I-1080p.mpd";

/*
 * Sequential Download
 * */
int nb_segments = 2;
int nb_rounds = 1;

std::vector<double> v_delay;
std::vector<double> v_throughput;

void sequential_download()
{
    std::string datafile = "analysis/1-picoquic-delay-estimate/picoquic_estimate_delay_throughput_big.dat";
    ChStreamOutAsciiFile mdatafile(datafile.c_str());

    int path_id = 0;
    printf("sequential download\n");

    int count = 0;
    for (int j = 0; j < nb_rounds; j++) {
        for (int i = 1; i < nb_segments; i++) {
            string filename = string("/1080/").append(urls[0][i]);
            printf("%s\n", filename.c_str());

            quic_config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
            picoquic_config_init(quic_config);
            quic_config->out_dir = "./tmp";

            struct picoquic_download_stat picoquic_st { };
            char* if_name = path_name[path_id];

            int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), if_name, &picoquic_st);
            printf("download ret = %d\n", ret);

            // this is rtt
            double delay = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
            printf("estimated delay: %f\n", delay);

            double throughput = picoquic_st.throughput;
            mdatafile << count << ", " << i << ", " << delay << ", " << throughput << "\n";

            free(quic_config);
            count++;

            v_delay.push_back(delay);
            v_throughput.push_back(throughput);
        }
    }

//    ChGnuPlot mplot;
//    string plot_png = datafile + "-picoquic_estimated_delay_rtt.png";
//    mplot.SetGrid();
//    mplot.SetLabelX("segments");
//    mplot.SetLabelY("picoquic_estimated_rtt");
//    mplot.OutputPNG(plot_png);
//    mplot.Plot(datafile, 1, 3, "picoquic estimated rtt", " with linespoints");
//
//    printf("average delay: %f\n", std::accumulate(v_delay.begin(), v_delay.end(), 0.0) / v_delay.size());
//    printf("max delay: %f\n", *std::max_element(v_delay.begin(), v_delay.end()));
//    printf("min delay: %f\n", *std::min_element(v_delay.begin(), v_delay.end()));

    ChGnuPlot mplot;
    string plot_png = datafile + "-picoquic_estimated_throughput_big.png";
    mplot.SetGrid();
    mplot.SetLabelX("segments");
    mplot.SetLabelY("picoquic estimated throughput");
    mplot.OutputPNG(plot_png);
    mplot.Plot(datafile, 2, 4, "picoquic estimated throughput big file", " with points");

    printf("average throughput: %f\n", std::accumulate(v_throughput.begin(), v_throughput.end(), 0.0) / v_throughput.size());
    printf("max throughput: %f\n", *std::max_element(v_throughput.begin(), v_throughput.end()));
    printf("min throughput: %f\n", *std::min_element(v_throughput.begin(), v_throughput.end()));
}


int main(int argc, char* argv[])
{
    mpd_file = parse_mpd(local_mpd_url, quic_config);
    urls = get_segment_urls(mpd_file);

    sequential_download();

    return 0;
}
