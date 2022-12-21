//
// Created by clarkzjw on 12/19/22.
//

#include "tplayer.h"

// http://concert.itec.aau.at/SVCDataset/
// Variant I is used
vector<int> bitrate_360{600, 990, 1500, 2075};
vector<int> bitrate_720{1500, 2750, 4800, 7800};
vector<int> bitrate_1080{4000, 5500, 7200, 10400};

map<int, vector<int>> map_bitrate{{360, bitrate_360}, {720, bitrate_720}, {1080, bitrate_1080}};
extern int cur_bitrate;
extern int cur_resolution;

int get_resolution_by_bitrate(int bitrate) {
    // http://concert.itec.aau.at/SVCDataset/
    // for Variant I, there are two resolutions that have the same bitrate,
    // in this case, we assume the higher resolution is preferred for a temporary solution
    if (bitrate == 1500) {
        return 720;
    }
    for (auto& [resolution, bitrates] : map_bitrate) {
        for (auto& b : bitrates) {
            if (b == bitrate) {
                return resolution;
            }
        }
    }
}


int get_next_resolution(int b) {
    int resolution = get_resolution_by_bitrate(b);
    if (resolution == 360) {
        return 720;
    } else if (resolution == 720) {
        return 1080;
    } else {
        return 1080;
    }
}

int get_next_bitrate_from_mapping(int b) {
    for (auto& [resolution, bitrates] : map_bitrate) {
        for (int i = 0; i < bitrates.size(); i++) {
            if (bitrates[i] == b) {
                if (i == bitrates.size() - 1) {
                    if (b != 10400) {
                        for (int j = 0; j < map_bitrate[get_next_resolution(b)].size(); j++) {
                            if (map_bitrate[get_next_resolution(b)][j] > b) {
                                return map_bitrate[get_next_resolution(b)][j];
                            }
                        }
                    } else {
                        return 10400;
                    }
                } else {
                    return bitrates[i + 1];
                }
            }
        }
    }
}



int global_get_highest_bitrate() {
    int highest_bitrate = 0;
    for (auto& [resolution, bitrates] : map_bitrate) {
        for (auto& b : bitrates) {
            if (b > highest_bitrate) {
                highest_bitrate = b;
            }
        }
    }
    return highest_bitrate;
}

int get_required_layer_by_bitrate(int bitrate) {
    int resolution = get_resolution_by_bitrate(bitrate);
    int layer = 1;
    for (auto& b : map_bitrate[resolution]) {
        if (b == bitrate) {
            return layer;
        }
        layer++;
    }
}

int initial_bitrate = 1500;
int initial_resolution = get_resolution_by_bitrate(initial_bitrate);

extern vector<struct reward_item> history_rewards;


double get_previous_average_reward() {
    double average = 0;
    if (history_rewards.empty()) {
        return average;
    }
    for (auto history_reward : history_rewards) {
        average += history_reward.reward;
    }
    return average / history_rewards.size();
}


double get_previous_average_reward_on_path_i(int path_id) {
    double average = 0;
    int count = 0;
    for (auto history_reward : history_rewards) {
        if (history_reward.path_id == path_id) {
            average += history_reward.reward;
            count ++;
        }
    }
    if (history_rewards.empty() || count == 0) {
        return average;
    }
    return average / count;
}

int decide_next_bitrate(int cur_reward) {
    int next_bitrate = cur_bitrate;
    if (history_rewards.empty()) {
        return next_bitrate;
    }
    double previous_average_reward = get_previous_average_reward();
    if (cur_reward > previous_average_reward) {
        next_bitrate = get_next_bitrate_from_mapping(cur_bitrate);
    } else {
        next_bitrate = cur_bitrate;
    }
    if (next_bitrate > global_get_highest_bitrate()) {
        next_bitrate = global_get_highest_bitrate();
    }
    return next_bitrate;
}

int decide_next_bitrate_path_i(int cur_reward, int path_id) {
    int next_bitrate = cur_bitrate;
    if (history_rewards.empty()) {
        return next_bitrate;
    }
    double previous_average_reward = get_previous_average_reward_on_path_i(path_id);
    if (cur_reward > previous_average_reward) {
        next_bitrate = get_next_bitrate_from_mapping(cur_bitrate);
    } else {
        next_bitrate = cur_bitrate;
    }
    if (next_bitrate > global_get_highest_bitrate()) {
        next_bitrate = global_get_highest_bitrate();
    }
    return next_bitrate;
}
