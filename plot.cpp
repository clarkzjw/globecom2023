//
// Created by clarkzjw on 1/9/23.
//

#include "tplayer.h"

string results_dir = "./result/";

int save_metrics_to_file() {
    string path = results_dir + experiment_id;
    ChStreamOutAsciiFile dat(path.c_str());

    dat << "seg_no" << "\t" \
    << "buffering_ratio" << "\t" \
    << "download_via_path_id" << "\t" \
    << "rtt_on_path" << "\t" \
    << "throughput_on_path" << "\t" \
    << "bitrate" << "\t" \
    << "resolution" << "\t" \
    << "reward" << "\n";

    return 0;
}
