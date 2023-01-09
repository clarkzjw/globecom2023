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

    for (int i = 0; i < seg_stats.size(); i++) {
        int seg_no = i + 1;
        double buffering_ratio = tmp_reward_map[i+1].buffering_ratio;
        int path_id = tmp_reward_map[i+1].path_id;
        double rtt = tmp_reward_map[i+1].rtt;
        double throughput = seg_stats[i+1].download_speed;
        double bitrate = tmp_reward_map[i+1].bitrate;
        int resolution = seg_stats[i+1].resolution;
        double reward = tmp_reward_map[i+1].reward;

        dat << seg_no << "\t" << buffering_ratio << "\t" << path_id << "\t" << rtt << "\t" \
        << throughput << "\t" << bitrate << "\t" << resolution << "\t" << reward << "\n";
    }
    return 0;
}
