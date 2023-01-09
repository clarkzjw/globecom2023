//
// Created by clarkzjw on 1/6/23.
//

#include "tplayer.h"

std::vector<BufferEvent> buffer_events_vec;

double previous_buffering_time_on_path(int path_id) {
    double previous_buffering_time = 0.0;
    for (auto& be : buffer_events_vec) {
        if (be.path_id == path_id && be.completed == 1) {
            previous_buffering_time += epoch_to_relative_seconds(be.start, be.end);
        } else if (be.path_id == path_id && be.completed == 0) {
            previous_buffering_time += epoch_to_relative_seconds(be.start, Tic());
        }
    }
    return previous_buffering_time;
}

double previous_total_buffering_time() {
    double previous_buffering_time = 0.0;
    for (auto& be : buffer_events_vec) {
        if (be.completed == 1) {
            previous_buffering_time += epoch_to_relative_seconds(be.start, be.end);
        } else if (be.completed == 0) {
            previous_buffering_time += epoch_to_relative_seconds(be.start, Tic());
        }
    }
    return previous_buffering_time;
}

double buffering_event_count_ratio_on_path(int path_id) {
    double count = 0;
    for (auto& be : buffer_events_vec) {
        if (be.path_id == path_id) {
            count ++;
        }
    }
    return count / (double)buffer_events_vec.size();
}
