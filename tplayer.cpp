//
// Created by Jinwei Zhao on 2022-08-12.
//

#include "tplayer.h"
#include "picoquic_config.h"
#include "picoquic_internal.h"
#include "picoquic_packet_loop.h"
#include "tictoc.h"
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <map>
#include "client.h"

typedef std::chrono::system_clock tic_clock;

#define PortableSleep(seconds) usleep((seconds)*1000000)
#define FRAME int
#define buffer_size 10

string py3exec = "python3";

string init_segment_url;
string mpd_url;
string default_mpd_url = "quic://server.local.me:4433/1080/BBB-I-1080p.mpd";
string local_mpd_url = "./BBB-I-1080p.mpd";
dash::mpd::IMPD* mpd_file;
vector<vector<string>> urls;

string host;
int port;
int stream_id = 0;

picoquic_quic_config_t* quic_config = nullptr;
queue<FRAME> buffer;
auto level = 3; // 0, 1, 2, 3

void sequential_download() {
    for (int i = 1; i < 10; i++) {
        string filename = string("/1080/").append(urls[0][i]);
        printf("%s\n", filename.c_str());

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "", NULL);
        printf("download ret = %d\n", ret);
    }
}

int enable_multipath = 1;
char const* multipath_links = "10.0.2.2/2,10.0.3.2/3";
char const* multipath_fast_link = "10.0.2.2/2";
char const* multipath_slow_link = "10.0.3.2/3";

void multipath_picoquic_builtin_minRTT_download() {
    quic_config->multipath_option = 2;
    quic_config->multipath_alt_config = (char *)malloc(sizeof(char) * (strlen(multipath_links) + 1));
    memset(quic_config->multipath_alt_config, 0, sizeof(char) * (strlen(multipath_links) + 1));
    memcpy(quic_config->multipath_alt_config, multipath_links, sizeof(char) * (strlen(multipath_links) + 1));
    printf("config.multipath_alt_config: %s\n", quic_config->multipath_alt_config);

    int nb_segments = (int)urls[0].size();
    int layers = (int)urls.size();

    for (int i = 1; i < 5; i++) {
        string filename;
        for (int j = 0; j < layers; j++) {
            filename += string("/1080/").append(urls[j][i]);
            filename += ";";
        }
        printf("%s\n", filename.c_str());

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "", NULL);
        printf("download ret = %d\n", ret);
    }
}

struct DownloadTask {
    string filename;
    int seg_no{};
    int eos{};
};

std::queue<DownloadTask> tasks;

string path_name[2] = {"h2-eth0", "h2-eth1"};


struct DownloadStats {
    string filename;
    int seg_no{};
    double time{}; // seconds
    double speed{};  // throughput measured by picoquic, Mbps
    double actual_speed{}; // Mbps
    int filesize{}; // bytes
    int path_index{};
};

tic_clock::time_point playback_start;

struct PerSegmentStats {
    tic_clock::time_point start;
    tic_clock::time_point end;
    int finished_layers{};
};

struct PlayableSegment {
    int eos{};
    int nb_frames;
};


std::vector<DownloadStats> stats;
std::map<int, PerSegmentStats> seg_stats;
std::queue<PlayableSegment> player_buffer;


int get_filesize(const string& req_filename) {
    std::string default_dir = "./tmp/";
    std::string a = req_filename.substr(1);
    std::replace(a.begin(), a.end(), '/', '_');
    std::string path = default_dir + a;

    std::filesystem::path p{path};
    size_t realsize = std::filesystem::file_size(p);
    std::cout << "filesize: " << realsize << std::endl;
    return (int)realsize;
}

// create a downloader for each path
// each path poll available tasks at the same time
void path_downloader(int path_index) {
    while (true) {
        if (!tasks.empty()) {
            struct DownloadTask t = tasks.front();
            tasks.pop();

            if (t.eos == 1) {
                printf("path %d ends\n", path_index);
                break;
            } else {
                int ret = 0;
                string if_name = path_name[path_index];
                printf("\n\ndownloading %s on path %s\n", t.filename.c_str(), if_name.c_str());

                struct DownloadStats st;
                st.filename = t.filename;
                st.seg_no = t.seg_no;

                struct picoquic_download_stat picoquic_st{};

                PerSegmentStats pst;
                TicToc timer;

                pst.start = timer.tic();

                ret = quic_client(host.c_str(), port, quic_config, 0, 0, t.filename.c_str(), (char *)if_name.c_str(), &picoquic_st);

                pst.finished_layers = 1;

                // if this segment is shown for the first time
                if (!seg_stats.contains(t.seg_no)) {
                    seg_stats.insert({t.seg_no, pst});
                } else {
                    seg_stats[t.seg_no].end = timer.toc();
                    seg_stats[t.seg_no].finished_layers++;

                    if (seg_stats[t.seg_no].finished_layers == 4) {
                        PlayableSegment ps{};
                        ps.nb_frames = 48;
                        ps.eos = 0;
                        player_buffer.push(ps);
                    }
                }

                st.time = timer.elapsed();
                st.speed = picoquic_st.throughput;
                st.path_index = path_index;
                st.filesize = get_filesize(t.filename);
                st.actual_speed = st.filesize / st.time / 1000000.0 * 8.0;

                stats.push_back(st);

                printf("download ret = %d\n", ret);
            }
        }
    }
}


void multipath_round_robin_download_queue() {
    std::thread t1(path_downloader, 0);
    std::thread t2(path_downloader, 1);

    t1.join();
    t2.join();

    PlayableSegment ps{};
    ps.eos = 1;
    player_buffer.push(ps);
}

double epoch_to_relative_seconds(tic_clock::time_point start, tic_clock::time_point end) {
    return double(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1e6;
}


struct BufferEvent {
    // relative seconds during the playback of a buffering event
    tic_clock::time_point start;
    tic_clock::time_point end;
    int completed;
};

std::vector<BufferEvent> buffering;

void player() {
    double FPS = 24.0;

    TicToc buffering_timer;
    auto player_start = buffering_timer.tic();

    while (true) {
//        PortableSleep(5);
//        printf("......Playback buffer size: %lu, buffering event size: %lu\n", player_buffer.size(), buffering.size());
        if (!player_buffer.empty()) {
            if (!buffering.empty() && buffering[buffering.size()-1].completed == 0) {
                buffering[buffering.size()-1].end = buffering_timer.tic();
                buffering[buffering.size()-1].completed = 1;
            }
            struct PlayableSegment s = player_buffer.front();
            player_buffer.pop();

            // this is the last segment
            if (s.eos == 1) {
                break;
            }

            int frames = s.nb_frames;
            PortableSleep( 1 / FPS * frames);
        } else {
            if (buffering.empty() || buffering[buffering.size() - 1].completed == 1) {
                struct BufferEvent be{};
                be.start = buffering_timer.tic();

                buffering.push_back(be);
                printf("######## New buffering event added\n");
            } else {
                continue;
            }

        }
    }

    for (auto &be: buffering) {
        std::cout << "buffer event, start: " << epoch_to_relative_seconds(player_start, be.start) << ", end: " \
        << epoch_to_relative_seconds(player_start, be.end) << endl;
    }
}

// we need a queue to store the tasks
void multipath_round_robin_download() {
    int nb_segments = (int)urls[0].size();
    int layers = (int)urls.size();

    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(multipath_round_robin_download_queue);
    std::thread thread_player(player);

    for (int i = 1; i < 5; i++) {
        for (int j = 0; j < layers; j++) {
            string filename = string("/1080/").append(urls[j][i]);

            struct DownloadTask t;
            t.filename = filename;
            t.seg_no = i;
            t.eos = 0;

            tasks.push(t);
            printf("pushed %s\n", filename.c_str());
        }
    }

    struct DownloadTask finish;
    finish.eos = 1;
    tasks.push(finish);
    tasks.push(finish);
    thread_download.join();
    thread_player.join();

    printf("stats size: %zu\n", stats.size());
    for (auto & stat : stats) {
        printf("seg_no: %d, filename: %s, filesize: %d, time: %f, speed: %f, actual_speed: %f, path_index: %d\n", stat.seg_no, stat.filename.c_str(), stat.filesize, stat.time, stat.speed, stat.actual_speed, stat.path_index);
    }

    for (const auto& [key, value] : seg_stats)
    {
        std::cout << '[' << key << "] =" << " used: " << \
        double(std::chrono::duration_cast<std::chrono::microseconds>(value.end - value.start).count()) / 1e6 \
        << " seconds, " << "ends at " << epoch_to_relative_seconds(playback_start, value.end) \
        << " finished layers " << value.finished_layers << endl;
    }
}


void test_map() {
    std::map<int,char> example = {{1,'a'},{2,'b'}};

    for(int x: {2, 5}) {
        if(example.contains(x)) {
            std::cout << x << ": Found\n";
        } else {
            std::cout << x << ": Not found\n";
        }
    }
    printf("%c\n", example[2]);
    example[2] = 'c';
    printf("%c\n", example[2]);
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

    quic_config = (picoquic_quic_config_t *)malloc(sizeof(picoquic_quic_config_t));

    picoquic_config_init(quic_config);
    quic_config->out_dir = "./tmp";

//    sequential_download();
//    multipath_picoquic_builtin_minRTT_download();
    multipath_round_robin_download();


    return 0;
}
