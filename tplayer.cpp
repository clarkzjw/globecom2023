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
#include <cxxopts.h>

using namespace std;

string host;
int port;

std::vector<DownloadStats> stats;
std::map<int, PerSegmentStats> seg_stats;
std::queue<PlayableSegment> player_buffer;
std::map<int, PlayableSegment> player_buffer_map;
std::queue<DownloadTask> tasks;
std::chrono::system_clock::time_point playback_start;
picoquic_quic_config_t* quic_config = nullptr;
dash::mpd::IMPD* mpd_file;
vector<vector<struct SegmentInfo>> urls;

vector<string> path_ifname_vec{"h2-eth0", "h2-eth1"};

int PLAYER_BUFFER_MAX_SEGMENTS = 5;

char const* multipath_links = "10.0.2.2/2,10.0.3.2/3";

string local_mpd_url = "../dataset/BigBuckBunny/mpd/stream.mpd";

auto level = 3; // 0, 1, 2, 3

// max available segments: 299
int nb_segments = 20;

extern double initial_bitrate;
//extern int initial_resolution;

string alg;
Algorithm scheduling_algorithm;

bool check_parameters(cxxopts::Options& options, int argc, char **argv) {
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (result.count("host")) {
        host = result["host"].as<string>();
    } else {
        host = "127.0.0.1";
    }

    if (result.count("port")) {
        port = result["port"].as<int>();
    } else {
        port = 4433;
    }

    auto hashit = [](const string& opt) -> Algorithm {
        if (opt == "mab") { return Algorithm::mab; }
        if (opt == "rr") { return Algorithm::rr; }
        if (opt == "pseudo_rr") { return Algorithm::pseudo_rr; }
        if (opt == "minrtt") { return Algorithm::minrtt; }
        return Algorithm::unexpected;
    };

    auto check_algorithm = [&hashit](const string& opt) {
        scheduling_algorithm = hashit(opt);
        switch (scheduling_algorithm) {
            case Algorithm::mab:
            case Algorithm::pseudo_rr:
            case Algorithm::rr:
            case Algorithm::minrtt:
                return true;
            case Algorithm::unexpected:
            default:
                cout << "Algorithm " << opt << " doesn't exist" << endl;
                return false;
        }
    };

    if (result.count("mpd")) {
        local_mpd_url = result["mpd"].as<string>();
    }

    if (result.count("algorithm")) {
        return check_algorithm(result["algorithm"].as<string>());
    }

    return true;
}

int main(int argc, char* argv[])
{
    cxxopts::Options options("tplayer", "MPQUIC Player");
    options.add_options()
            ("p,port", "server port", cxxopts::value<int>())
            ("h,host", "server host", cxxopts::value<string>())
            ("a,algorithm", "test algorithm", cxxopts::value<string>())
            ("m,mpd", "mpd file path", cxxopts::value<string>())
            ("help", "print usage")
            ;

    if (!check_parameters(options, argc, argv)) {
        return 1;
    }

    cout << "host: " << host << ", port: " << port << ", algorithm: " << alg << ", mpd: " << local_mpd_url << endl;

    // read mpd file
    mpd_file = parse_mpd(local_mpd_url);
    urls = get_segment_urls(mpd_file);

    // start download and playback
    start();

    return 0;
}
