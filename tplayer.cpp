//
// Created by Jinwei Zhao on 2022-08-12.
//

#include "tplayer.h"

using namespace std;

int download_segment(char *_host, int _port, char *_filename, char *_if_name, const char *_dir, picoquic_download_stat *stat) {
    auto *config = (picoquic_quic_config_t*)malloc(sizeof(picoquic_quic_config_t));
    picoquic_config_init(config);
    config->out_dir = _dir;
    config->sni = "test";

    cout << "######## libplayer: Downloading " << _filename << " on " << _if_name << endl;
    int ret = quic_client(_host, _port, config, 0, 0, _filename, _if_name, stat);

    printf("download ret = %d\n", ret);
    return 0;
}
