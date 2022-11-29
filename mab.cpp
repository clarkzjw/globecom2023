//
// Created by clarkzjw on 11/11/22.
//
#include "tplayer.h"

#include "arm/arm.hpp"
#include "arm/arm_bernoulli.hpp"
#include "arm/arm_normal.hpp"
#include "bandit/simulator.hpp"
#include "bandit/util.hpp"
#include "policy/policy.hpp"
#include "policy/policy_dmed.hpp"
#include "policy/policy_egreedy.hpp"
#include "policy/policy_klucb.hpp"
#include "policy/policy_moss.hpp"
#include "policy/policy_random.hpp"
#include "policy/policy_ts.hpp"
#include "policy/policy_ucb.hpp"
#include "policy/policy_ucbv.hpp"

#include <cassert>

using namespace std;
using namespace bandit;

void multipath_mab_path_scheduler()
{
    int nb_segments = 100;
    double epsilon = 0.5;

    //arms.size();
    const uint K = 2;

    //policies.size()
    const uint P = 1;

    RoundwiseLog log(K, P, nb_segments);
    vector<ArmPtr> arms;
    vector<PolicyPtr> policies;

    arms.clear();
    arms.push_back(ArmPtr(new BernoulliArm(0.05)));
    arms.push_back(ArmPtr(new BernoulliArm(0.1)));
    assert(arms.size() == K);

    policies.clear();
    policies.push_back(PolicyPtr(new EGreedyPolicy(K, epsilon)));
    assert(policies.size() == P);

    Simulator<RoundwiseLog> sim(arms, policies);
    int layers = (int)urls.size();

    for (int i = 1; i < nb_segments; i++) {
        string filename;

        for (int j = 0; j < layers; j++) {
            filename += string("/1080/").append(urls[j][i]);
            filename += ";";
        }
        printf("%s\n", filename.c_str());

        // per segment path decision
        int path_id = sim.select_next_path(0);

        char* if_name = path_name[path_id];
        struct picoquic_download_stat picoquic_st { };

        struct DownloadStats st;
        st.filename = filename;
        st.seg_no = i;

        PerSegmentStats pst;
        TicToc timer;

        pst.start = timer.tic();

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), if_name, &picoquic_st);
        pst.end = timer.tic();

        printf("download ret = %d\n", ret);

        double reward = picoquic_st.throughput;

        sim.set_reward(reward, 0, path_id);

        printf("selected path: %d, reward: %f\n", path_id, reward);

        if (!seg_stats.contains(i)) {
            for (int j = 0; j < layers; j++) {
                string tmp_filename = string("/1080/").append(urls[j][i]);
                pst.file_size += get_filesize(tmp_filename);
            }
            pst.finished_layers = 4;
            pst.reward = reward;
            pst.path_id = path_id;

            PlayableSegment ps {};
            ps.nb_frames = 48;
            ps.eos = 0;
            ps.seg_no = i;
            player_buffer.push(ps);

            seg_stats.insert({ i, pst });
        }
    }

    PlayableSegment ps {};
    ps.eos = 1;
    player_buffer.push(ps);
}

void multipath_mab()
{
    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(multipath_mab_path_scheduler);
    std::thread thread_playback(mock_player);

    thread_download.join();
    thread_playback.join();

    std::string datafile = "mab_EGreedyPolicy_0.5.dat";

    ChStreamOutAsciiFile mdatafile(datafile.c_str());

    printf("\nper segment download metrics\n");
    for (auto& [seg_no, value] : seg_stats) {
        value.download_time = double(std::chrono::duration_cast<std::chrono::microseconds>(value.end - value.start).count()) / 1e6;
        value.download_speed = value.file_size / value.download_time / 1000000.0 * 8.0;

        mdatafile << seg_no << ", " << value.download_speed << "\n";
        std::cout << seg_no << " = "
                  << "used: " << value.download_time
                  << " seconds, "
                  << "ends at " << epoch_to_relative_seconds(playback_start, value.end)
                  << " speed: " << value.download_speed << " Mbps"
                  << " total filesize " << value.file_size << " bytes"
                  << " path_id: " << value.path_id
                  << " reward: " << value.reward
                  << endl;
    }

    ChGnuPlot mplot;
    string plot_png = datafile + ".png";
    mplot.SetGrid();
    mplot.SetLabelX("segments");
    mplot.SetLabelY("throughput Mbps");
    mplot.OutputPNG(plot_png);

    mplot.Plot(datafile, 1, 2, "throughput", " with linespoints");
}
