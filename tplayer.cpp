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
#include "client.h"

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
double FPS = 24;
auto level = 3; // 0, 1, 2, 3

void sequential_download() {
    for (int i = 1; i < 10; i++) {
        string filename = string("/1080/").append(urls[0][i]);
        printf("%s\n", filename.c_str());

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "");
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

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "");
        printf("download ret = %d\n", ret);
    }
}

struct DownloadTask {
    string filename;
    int eos{};
};

std::queue<DownloadTask> tasks;

string path_name[2] = {"h2-eth0", "h2-eth1"};


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

                ret = quic_client(host.c_str(), port, quic_config, 0, 0, t.filename.c_str(), (char *)if_name.c_str());
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
}


// we need a queue to store the tasks
void multipath_round_robin_download() {
    int nb_segments = (int)urls[0].size();
    int layers = (int)urls.size();

    std::thread thread_download(multipath_round_robin_download_queue);

    for (int i = 1; i < 5; i++) {
        for (int j = 0; j < layers; j++) {
            string filename = string("/1080/").append(urls[j][i]);

            struct DownloadTask t;
            t.filename = filename;
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
