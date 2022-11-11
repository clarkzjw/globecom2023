//
// Created by Jinwei Zhao on 2022-08-12.
//

#ifndef TPLAYER_TPLAYER_H
#define TPLAYER_TPLAYER_H

#include "libdash.h"
#include "tictoc.h"

#include "picoquic_config.h"
#include "picoquic_utils.h"
#include "picoquic_config.h"
#include "picoquic_internal.h"
#include "picoquic_packet_loop.h"
#include "client.h"


using namespace std;





dash::mpd::IMPD* parse_mpd(string& url, picoquic_quic_config_t* config);
vector<vector<string>> get_segment_urls(dash::mpd::IMPD* mpd_file);
int exec_cmd(string cmd, string args);



#endif // TPLAYER_TPLAYER_H
