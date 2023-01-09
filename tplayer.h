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
#include "tictoc.h"

using namespace std;
using namespace ::chrono;
using namespace ::chrono::postprocess;


//typedef std::chrono::system_clock tic_clock;
#define PortableSleep(seconds) usleep((seconds)*1000000)

struct DownloadTask {
    string filename;
    int layers;
    int bitrate_level;
    int resolution;
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
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
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
    double latest_avg_rtt;

    double gamma;
    double gamma_throughput;
    double gamma_avg_throughput;
    double gamma_rtt;
    double gamma_avg_rtt;


    double cur_rtt;
};

struct PlayableSegment {
    int eos {};
    int nb_frames {};
    int seg_no {};
    double duration_seconds{};
};

struct SegmentPlaybackInfo {
    int seg_no;
    double playback_start_second;
    double playback_end_second;
    double playback_duration;
};

struct BufferEvent {
    // relative seconds during the playback of a buffer_events_vec event
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    int completed{};
    int path_id{};
    int next_seg_no{};
};

struct SegmentInfo {
    string url;
    int duration;
    double duration_seconds;
};

extern vector<vector<struct SegmentInfo>> urls;
extern std::chrono::system_clock::time_point playback_start;
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
extern vector<string> path_ifname_vec;
extern std::vector<DownloadStats> stats;

int get_filesize(const string& req_filename);
double epoch_to_relative_seconds(std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end);

dash::mpd::IMPD* parse_mpd(string& url);
vector<vector<struct SegmentInfo>> get_segment_urls(dash::mpd::IMPD* mpd_file);
int exec_cmd(string cmd, string args);

void mock_player();
void main_player_mock();

void multipath_mab();
void multipath_picoquic_minRTT();
void multipath_round_robin();

#define nb_paths 2

int get_next_bitrate_from_mapping(int b);
int decide_next_bitrate(double cur_reward);
int decide_next_bitrate_path_i(double cur_reward, double avg, int path_id);
int get_nearest_bitrate(double reward);
double get_next_bitrate(double b, double reward, double previous_reward);

double get_previous_average_reward();
double get_previous_average_reward_on_path_i(int path_id);
double get_previous_most_recent_average_reward_on_path_i(int path_id);
double get_previous_most_recent_average_reward(int nb_previous);

int get_resolution_by_bitrate(double bitrate);
int global_get_highest_bitrate();
int get_required_layer_by_bitrate(int bitrate);
double get_latest_total_throughput();
double get_latest_average_rtt();


double get_previous_average_throughput(int path_id);
double get_previous_average_rtt(int path_id, int nb_segments);

struct reward_item {
    double reward;
    int path_id;
};

void start();

double get_bitrate_from_bitrate_level(int level);
double previous_buffering_time_on_path(int path_id);
double previous_total_buffering_time();

#define TPLAYER_DEBUG 1

enum class Algorithm{
    pseudo_rr,
    rr,
    mab,
    minrtt,
    unexpected,
};

typedef std::function<void(int, const struct DownloadTask, std::mutex* path_mutex)> CallbackDownload;
double buffering_event_count_ratio_on_path(int path_id);


#endif // TPLAYER_TPLAYER_H
