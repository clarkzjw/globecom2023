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
