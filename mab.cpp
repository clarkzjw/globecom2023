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
#include <vector>
#include <future>
#include <mutex>

using namespace std;
using namespace bandit;

extern int nb_segments;

vector<queue<DownloadTask>> path_tasks(2);
vector<mutex> path_mutex(2);


void mab_path_downloader(int path_index)
{
    int layers = (int)urls.size();

    while (true) {
        if (!path_tasks[path_index].empty()) {
            while (player_buffer.size() > PLAYER_BUFFER_MAX_SEGMENTS) {
                PortableSleep(0.1);
            }
            struct DownloadTask t = path_tasks[path_index].front();
            path_tasks[path_index].pop();

            if (t.eos == 1) {
                printf("path %d ends\n", path_index);
                break;
            } else {
                int ret = 0;
                char* if_name = path_name[path_index];
                printf("\n\ndownloading %s on path %s\n", t.filename.c_str(), if_name);

                struct picoquic_download_stat picoquic_st { };

                PerSegmentStats pst;
                TicToc timer;

                pst.start = timer.tic();
                ret = quic_client(host.c_str(), port, quic_config, 0, 0, t.filename.c_str(), if_name, &picoquic_st);
                pst.end = timer.tic();

                cout << "download ret = " << ret << endl;
                if (!seg_stats.contains(t.seg_no)) {
                    for (int j = 0; j < layers; j++) {
                        string tmp_filename = string("/1080/").append(urls[j][t.seg_no]);
                        pst.file_size += get_filesize(tmp_filename);
                    }
                    pst.finished_layers = 4;
                    pst.path_id = path_index;
                    pst.reward = picoquic_st.throughput;
                    pst.one_way_delay_avg = picoquic_st.one_way_delay_avg;
                    pst.bandwidth_estimate = picoquic_st.bandwidth_estimate;
                    pst.rtt = picoquic_st.rtt;
                    pst.total_received = picoquic_st.total_received;
                    pst.total_bytes_lost = picoquic_st.total_bytes_lost;
                    pst.data_received = picoquic_st.data_received;


                    PlayableSegment ps {};
                    ps.nb_frames = 48;
                    ps.eos = 0;
                    ps.seg_no = t.seg_no;
                    player_buffer_map[t.seg_no] = ps;

                    seg_stats.insert({ t.seg_no, pst });
                }
            }
        }
    }
}


void mab_path_downloader_new(int path_id, struct DownloadTask t, Simulator<RoundwiseLog> sim)
{
    int ret = 0;
    int layers = (int)urls.size();

//    while (player_buffer.size() > PLAYER_BUFFER_MAX_SEGMENTS) {
//        PortableSleep(0.1);
//    }
//    struct DownloadTask t = path_tasks[path_index].front();
//    path_tasks[path_index].pop();

    char* if_name = path_name[path_id];
    printf("\n\ndownloading %s on path %s\n", t.filename.c_str(), if_name);

    struct picoquic_download_stat picoquic_st { };
    PerSegmentStats pst;
    TicToc timer;

    pst.start = timer.tic();
    ret = quic_client(host.c_str(), port, quic_config, 0, 0, t.filename.c_str(), if_name, &picoquic_st);
    pst.end = timer.tic();

    cout << "download ret = " << ret << endl;
    if (!seg_stats.contains(t.seg_no)) {
        for (int j = 0; j < layers; j++) {
            string tmp_filename = string("/1080/").append(urls[j][t.seg_no]);
            pst.file_size += get_filesize(tmp_filename);
        }
        pst.finished_layers = 4;
        pst.path_id = path_id;

        pst.one_way_delay_avg = picoquic_st.one_way_delay_avg;
        pst.rtt_delay_estimate = ((double)picoquic_st.one_way_delay_avg) / 1000.0 * 2;
        pst.download_speed = picoquic_st.throughput;

        pst.bandwidth_estimate = picoquic_st.bandwidth_estimate;
        pst.rtt = picoquic_st.rtt;
        pst.total_received = picoquic_st.total_received;
        pst.total_bytes_lost = picoquic_st.total_bytes_lost;
        pst.data_received = picoquic_st.data_received;

        // reward function
        double alpha = 1;
        double beta = 1;
//        double gamma = 1;

        double reward = alpha * picoquic_st.throughput + beta / pst.rtt_delay_estimate;

        pst.reward = reward;
//        pst.reward = picoquic_st.throughput;

        PlayableSegment ps {};
        ps.nb_frames = 48;
        ps.eos = 0;
        ps.seg_no = t.seg_no;
        player_buffer_map[t.seg_no] = ps;
        player_buffer.push(ps);

        seg_stats.insert({ t.seg_no, pst });
    }
    sim.set_reward(pst.reward, 0, path_id);
}


void multipath_mab_path_scheduler()
{
    double epsilon = 0.5;
    //arms.size();
    const uint K = 2;
    //policies.size()
    const uint P = 1;

    int nb_paths = 2;

    RoundwiseLog log(K, P, nb_segments);
    vector<ArmPtr> arms;
    vector<PolicyPtr> policies;

    arms.push_back(ArmPtr(new BernoulliArm(0.05)));
    arms.push_back(ArmPtr(new BernoulliArm(0.1)));
    assert(arms.size() == K);

//    policies.push_back(PolicyPtr(new EGreedyPolicy(K, epsilon)));
    policies.push_back(PolicyPtr(new ThompsonBinaryPolicy(K)));
    assert(policies.size() == P);

    Simulator<RoundwiseLog> sim(arms, policies);
    int layers = (int)urls.size();

    BS::thread_pool download_thread_pool(nb_paths);

    int i = 1;
    while (true) {
        if (i >= nb_segments) {
            break;
        }

        if (player_buffer.size() >= PLAYER_BUFFER_MAX_SEGMENTS || download_thread_pool.get_tasks_total() >= PLAYER_BUFFER_MAX_SEGMENTS) {
            PortableSleep(0.01);
            continue;
        }

        string filename;
        for (int j = 0; j < layers; j++) {
            filename += string("/1080/").append(urls[j][i]);
            filename += ";";
        }
        printf("%s\n", filename.c_str());

        // per segment path decision
        int path_id = sim.select_next_path(0);

        struct DownloadTask t;
        t.filename = filename;
        t.seg_no = i;

        download_thread_pool.push_task(mab_path_downloader_new, path_id, t, sim);
        i++;
    }

    struct PlayableSegment ps;
    ps.eos = 1;
    player_buffer.push(ps);
    player_buffer_map[nb_segments] = ps;
}

void multipath_mab()
{
    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(multipath_mab_path_scheduler);
    std::thread thread_playback(mock_player_hash_map);

    thread_download.join();
    thread_playback.join();

    std::string datafile = "mab_ts.dat";

    ChStreamOutAsciiFile mdatafile(datafile.c_str());

    printf("\nper segment download metrics\n");
    for (auto& [seg_no, value] : seg_stats) {
        value.download_time = double(std::chrono::duration_cast<std::chrono::microseconds>(value.end - value.start).count()) / 1e6;
//        value.download_speed = value.file_size / value.download_time / 1000000.0 * 8.0;

        mdatafile << seg_no << ", " << value.download_speed << ", " << value.reward << "\n";
        std::cout << seg_no << "="
                  << " used: " << value.download_time << " seconds,"
                  << " ends at " << epoch_to_relative_seconds(playback_start, value.end)
                  << " speed: " << value.download_speed << " Mbps"
                  << " total filesize " << value.file_size << " bytes"
                  << " path_id: " << value.path_id
                  << " reward: " << value.reward
                  << " rtt: " << value.rtt_delay_estimate
//                  << " bw: " << value.bandwidth_estimate
//                  << " one way delay avg " << value.one_way_delay_avg
                  << " total bytes lost " << value.total_bytes_lost
                  << " total received " << value.total_received
                  << " data received " << value.data_received
                  << endl;
    }

//    ChGnuPlot mplot_throughput;
//    string plot_png = datafile + ".png";
//    mplot_throughput.SetGrid();
//    mplot_throughput.SetLabelX("segments");
//    mplot_throughput.SetLabelY("throughput/Mbps");
//    mplot_throughput.OutputPNG(plot_png);
//    mplot_throughput.Plot(datafile, 1, 2, "throughput", " with linespoints");
//    printf("plot saved to %s\n", plot_png.c_str());

    ChGnuPlot mplot_reward;
    string plot_reward_png = datafile + "-reward.png";
    mplot_reward.SetGrid();
    mplot_reward.SetLabelX("segments");
    mplot_reward.SetLabelY("rewards");
    mplot_reward.OutputPNG(plot_reward_png);
    mplot_reward.Plot(datafile, 1, 3, "rewards", " with linespoints");
    printf("plot saved to %s\n", plot_reward_png.c_str());
}
