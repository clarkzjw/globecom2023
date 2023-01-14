//
// Created by clarkzjw on 1/9/23.
//

#include "tplayer.h"
#include <matplot/matplot.h>

string results_dir = "./result/";

extern string experiment_id;

typedef struct PlotMetric {
    vector<double> reward;
    vector<double> rtt;
} PlotMetric;


int plot_metrics(PlotMetric& pm) {
    using namespace matplot;

    std::vector<int> seg_no(pm.reward.size());
    std::iota (std::begin(seg_no), std::end(seg_no), 1);

    plot(seg_no, pm.reward, "-o");
    xlabel("Playback Segment Number");
    ylabel("Reward");
    hold(on);

    plot(seg_no, pm.rtt, "-o")->use_y2(true);
    y2label("RTT");
    y2lim({0, 200});
    title("minRTT");

    save(results_dir + experiment_id+".png", "png");
    return 0;
}


int save_metrics_to_file() {
    string path = results_dir + experiment_id;
    std::ofstream dat (path.c_str());

    dat << "seg_no" << "\t" \
    << "buffering_ratio" << "\t" \
    << "download_via_path_id" << "\t" \
    << "rtt_on_path" << "\t" \
    << "throughput_on_path" << "\t" \
    << "bitrate" << "\t" \
    << "resolution" << "\t" \
    << "reward" << "\n";

    PlotMetric pm;

    for (int i = 0; i < seg_stats.size(); i++) {
        int seg_no = i + 1;
        double buffering_ratio = 1.0 / tmp_reward_map[i+1].buffering_ratio;
        int path_id = tmp_reward_map[i+1].path_id;
        pm.rtt.push_back(tmp_reward_map[i+1].rtt);
        double throughput = seg_stats[i+1].download_speed;
        double bitrate = tmp_reward_map[i+1].bitrate;
        int resolution = seg_stats[i+1].resolution;
        pm.reward.push_back(tmp_reward_map[i+1].reward);

        dat << seg_no << "\t" << buffering_ratio << "\t" << path_id << "\t" << pm.rtt[i] << "\t" \
        << throughput << "\t" << bitrate << "\t" << resolution << "\t" << pm.reward[i] << "\n";
    }

    dat.close();

    plot_metrics(pm);

    return 0;
}
