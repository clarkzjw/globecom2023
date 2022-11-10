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
    for (int i = 1; i < 4; i++) {
        string filename = string("/1080/").append(urls[0][i]);
        printf("%s\n", filename.c_str());

    }
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

    sequential_download();

    return 0;
}
