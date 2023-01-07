//
// Created by clarkzjw on 12/19/22.
//

#include "tplayer.h"

// bitarte_mapping = {
//     1080: {
//         1: 87209.263,
//         2: 71817.751,
//         ...
//     },
//     720: {
//         9: 35902.455,
//         ...
//     }
// }

map<int, map<int, double>> bitrate_mapping{
        {1080, {
                       {1,  87209.263},
                       {2,  71817.751},
                       {3,  55301.205},
                       {4,  39808.113},
                       {5,  27377.193},
                       {6,  18436.293},
                       {7,  13157.833},
                       {8,  9907.154}
               }
        },
        {
         720,  {
                       {9,  35902.455},
                       {10, 29301.111},
                       {11, 22572.278},
                       {12, 16521.917},
                       {13, 11818.484},
                       {14, 8433.333},
                       {15, 6239.562},
                       {16, 4791.332}
               }
        },
        {
         480,  {
                       {17, 15127.655},
                       {18, 12446.732},
                       {19, 9791.634},
                       {20, 7437.2},
                       {21, 5578.399},
                       {22, 4198.176},
                       {23, 3224.021},
                       {24, 2524.693}
               }
        },
        {
         360,  {
                       {25, 8533.59},
                       {26, 7116.171},
                       {27, 5719.322},
                       {28, 4481.84},
                       {29, 3486.456},
                       {30, 2719.651},
                       {31, 2149.588},
                       {32, 1716.847}
               }
        },
        {
         240,  {
                       {33, 3795.756},
                       {34, 3176.14},
                       {35, 2582.024},
                       {36, 2055.819},
                       {37, 1625.187},
                       {38, 1287.185},
                       {39, 1026.325},
                       {40, 822.111}
               }
        },
        {
         144,  {
                       {41, 1445.037},
                       {42, 1219.789},
                       {43, 1001.111},
                       {44, 809.758},
                       {45, 652.03},
                       {46, 526.099},
                       {47, 425.853},
                       {48, 344.696}
               }
        }

};

//double initial_bitrate = 1500;
//int initial_resolution = get_resolution_by_bitrate(initial_bitrate);

double initial_bitrate;
//int initial_resolution = 720;
//int initial_bitrate_level = 13;

double get_bitrate_from_bitrate_level(int level) {
    for (auto const& [resolution, bitrate_map] : bitrate_mapping) {
        for (auto const& [bitrate_level, bitrate_value] : bitrate_map) {
            if (bitrate_level == level) {
                return bitrate_value;
            }
        }
    }
    return -1;
}

int get_resolution_by_bitrate(double bitrate) {
    // loop through the bitrate_mapping and find the resolution by bitrate
    for (auto const& [resolution, bitrate_map] : bitrate_mapping) {
        for (auto const& [bitrate_level, bitrate_value] : bitrate_map) {
            if (bitrate_value == bitrate) {
                return resolution;
            }
        }
    }
    return -1;
}

// http://concert.itec.aau.at/SVCDataset/
// Variant I is used
vector<int> bitrate_360{600, 990, 1500, 2075};
vector<int> bitrate_720{1500, 2750, 4800, 7800};
vector<int> bitrate_1080{4000, 5500, 7200, 10400};

map<int, vector<int>> map_bitrate{{360,  bitrate_360},
                                  {720,  bitrate_720},
                                  {1080, bitrate_1080}};
extern double cur_bitrate;
extern int cur_resolution;

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
    for (auto &[resolution, bitrates]: map_bitrate) {
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
    // shouldn't reach here
    return -1;
}


//vector<int> bitrate_360{600, 990, 1500, 2075};
//vector<int> bitrate_720{1500, 2750, 4800, 7800};
//vector<int> bitrate_1080{4000, 5500, 7200, 10400};
//
//map<int, vector<int>> map_bitrate{{360, bitrate_360}, {720, bitrate_720}, {1080, bitrate_1080}};


int get_previous_bitrate_from_mapping(int b) {
    switch (b) {
        case 600:
            return 600;
        case 990:
            return 600;
        case 1500:
            return 990;
        case 2075:
            return 1500;
        case 2750:
            return 1500;
        case 4800:
            return 2750;
        case 7800:
            return 4800;
        case 4000:
            return 2750;
        case 5500:
            return 4000;
        case 7200:
            return 5500;
        case 10400:
            return 7200;
        default:
            return 10400;
    }
}


int global_get_highest_bitrate() {
    int highest_bitrate = 0;
    for (auto &[resolution, bitrates]: map_bitrate) {
        for (auto &b: bitrates) {
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
    for (auto &b: map_bitrate[resolution]) {
        if (b == bitrate) {
            return layer;
        }
        layer++;
    }
    // shouldn't reach here
    return -1;
}



extern vector<struct reward_item> history_rewards;


double get_previous_average_reward() {
    double average = 0;
    if (history_rewards.empty()) {
        return average;
    }
    for (auto history_reward: history_rewards) {
        average += history_reward.reward;
    }
    return average / history_rewards.size();
}


double get_previous_average_reward_on_path_i(int path_id) {
    double average = 0;
    int count = 0;
    for (auto history_reward: history_rewards) {
        if (history_reward.path_id == path_id) {
            average += history_reward.reward;
            count++;
        }
    }
    if (history_rewards.empty() || count == 0) {
        return average;
    }
    return average / count;
}

double get_previous_most_recent_average_reward_on_path_i(int path_id) {
    double average = 0;
    int count = 0;
    int previous_number = 1;

    for (int i = history_rewards.size() - 1; i >= 0; i--) {
        if (history_rewards[i].path_id == path_id) {
            average += history_rewards[i].reward;
            count++;
        }
        if (count == previous_number) {
            break;
        }
    }
    if (history_rewards.empty() || count == 0) {
        return average;
    }
    return average / count;
}


double get_previous_most_recent_average_reward(int nb_previous) {
    double average = 0;
    int count = 0;

    for (int i = history_rewards.size() - 1; i >= 0; i--) {
        average += history_rewards[i].reward;
        count++;
        if (count == nb_previous) {
            break;
        }
    }
    if (history_rewards.empty() || count == 0) {
        return average;
    }
    return average / count;
}

int decide_next_bitrate(double cur_reward) {
    int next_bitrate = cur_bitrate;
    if (history_rewards.empty()) {
        return next_bitrate;
    }
    double previous_average_reward = get_previous_average_reward();
    if (cur_reward >= previous_average_reward) {
        next_bitrate = get_next_bitrate_from_mapping(cur_bitrate);
    } else {
        next_bitrate = get_previous_bitrate_from_mapping(cur_bitrate);
//        next_bitrate = cur_bitrate;
    }
    if (next_bitrate > global_get_highest_bitrate()) {
        next_bitrate = global_get_highest_bitrate();
    }
    return next_bitrate;
}

int decide_next_bitrate_path_i(double cur_reward, double avg, int path_id) {
    int next_bitrate = cur_bitrate;
    if (history_rewards.empty()) {
        return next_bitrate;
    }
//    double previous_average_reward = get_previous_average_reward_on_path_i(path_id);
    if (cur_reward >= avg) {
        next_bitrate = get_next_bitrate_from_mapping(cur_bitrate);
    } else {
        next_bitrate = get_previous_bitrate_from_mapping(cur_bitrate);
//        next_bitrate = cur_bitrate;
    }
    if (next_bitrate > global_get_highest_bitrate()) {
        next_bitrate = global_get_highest_bitrate();
    }
    return next_bitrate;
}

int get_nearest_bitrate(double reward) {
    int nearest_bitrate = 0;
    double min_diff = 100000000;
    for (auto &[resolution, bitrates]: map_bitrate) {
        for (auto &b: bitrates) {
            double diff = abs(reward - b);
            if (diff < min_diff) {
                min_diff = diff;
                nearest_bitrate = b;
            }
        }
    }
    return nearest_bitrate;
}

int get_next_bitrate(double b, double reward, double previous_reward) {
    double next_bitrate = b;
    if (reward - previous_reward >= 0) {
        next_bitrate = get_next_bitrate_from_mapping(b);
    } else {
//        next_bitrate = b;
        if (reward / previous_reward > 0.8) {
            next_bitrate = b;
        } else {
            next_bitrate = get_previous_bitrate_from_mapping(b);
        }
    }
    if (next_bitrate > global_get_highest_bitrate()) {
        next_bitrate = global_get_highest_bitrate();
    }
    return next_bitrate;
}
