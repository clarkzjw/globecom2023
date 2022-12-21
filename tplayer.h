//
// Created by Jinwei Zhao on 2022-08-12.
//

#ifndef TPLAYER_TPLAYER_H
#define TPLAYER_TPLAYER_H

#include "libdash.h"

#include "client.h"
#include "picoquic_config.h"
#include "picoquic_internal.h"
#include "picoquic_packet_loop.h"
#include "picoquic_utils.h"

#include <queue>
#include <thread>

#include "chrono/core/ChLog.h"
#include "chrono/core/ChGlobal.h"
#include "chrono/motion_functions/ChFunction_Recorder.h"
#include "chrono/motion_functions/ChFunction_Sine.h"
#include "chrono_postprocess/ChGnuPlot.h"
#include "chrono_thirdparty/filesystem/path.h"

#include "BS_thread_pool.hpp"

using namespace std;
using namespace ::chrono;
using namespace ::chrono::postprocess;

#include "tictoc.h"

typedef std::chrono::system_clock tic_clock;
#define PortableSleep(seconds) usleep((seconds)*1000000)

struct DownloadTask {
    string filename;
    int layers;
    int seg_no {};
    int eos {};
};

struct DownloadStats {
    string filename;
    int seg_no {};
    double time {}; // seconds
    double speed {}; // throughput measured by picoquic, Mbps
    double actual_speed {}; // Mbps
    int filesize {}; // bytes
    int path_index {};
};

struct PerSegmentStats {
    tic_clock::time_point start;
    tic_clock::time_point end;
    int finished_layers {};
    int file_size {};
    double download_time {};
    double download_speed {};
    int path_id;
    uint64_t one_way_delay_avg;
    uint64_t bandwidth_estimate;
    uint64_t rtt;
    uint64_t total_bytes_lost;
    uint64_t total_received;
    uint64_t data_received;

    int resolution;
    int bitrate;

    double rtt_delay_estimate;
    double reward;
    double average_reward_so_far;
};

struct PlayableSegment {
    int eos {};
    int nb_frames {};
    int seg_no {};
};

struct BufferEvent {
    // relative seconds during the playback of a buffering event
    tic_clock::time_point start;
    tic_clock::time_point end;
    int completed;
};

extern vector<vector<string>> urls;
extern tic_clock::time_point playback_start;
extern picoquic_quic_config_t* quic_config;
extern char const* multipath_links;
extern std::queue<PlayableSegment> player_buffer;
extern std::map<int, PlayableSegment> player_buffer_map;
extern std::map<int, PerSegmentStats> seg_stats;
extern int PLAYER_BUFFER_MAX_SEGMENTS;

extern string host;
extern int port;
extern std::queue<DownloadTask> tasks;
extern char* path_name[2];
extern std::vector<DownloadStats> stats;

int get_filesize(const string& req_filename);
double epoch_to_relative_seconds(tic_clock::time_point start, tic_clock::time_point end);

dash::mpd::IMPD* parse_mpd(string& url, picoquic_quic_config_t* config);
vector<vector<string>> get_segment_urls(dash::mpd::IMPD* mpd_file);
int exec_cmd(string cmd, string args);

void mock_player();
void mock_player_hash_map();

void multipath_mab();
void multipath_picoquic_minRTT();
void multipath_round_robin();

#define nb_paths 2

int get_next_bitrate_from_mapping(int b);
int decide_next_bitrate(int cur_reward);
int decide_next_bitrate_path_i(int cur_reward, int path_id);
double get_previous_average_reward();
double get_previous_average_reward_on_path_i(int path_id);

int get_resolution_by_bitrate(int bitrate);
int global_get_highest_bitrate();
int get_required_layer_by_bitrate(int bitrate);
double get_latest_total_throughput();
double get_latest_average_rtt();

struct reward_item {
    double reward;
    int path_id;
};

#endif // TPLAYER_TPLAYER_H
