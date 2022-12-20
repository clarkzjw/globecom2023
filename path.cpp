//
// Created by clarkzjw on 12/19/22.
//

#include "tplayer.h"

vector<double> path_rtt[nb_paths];
vector<double> path_throughput[nb_paths];

vector<double> history_rewards;

double get_latest_total_throughput() {
    double throughput = 0;
    for (auto & path : path_throughput) {
        if (!path.empty()) {
            throughput += path[path.size() - 1];
        }
    }
    return throughput;
}

double get_latest_average_rtt() {
    double rtt = 0;
    for (auto & path : path_rtt) {
        if (!path.empty()) {
            rtt += path[path.size() - 1];
        }
    }
    return rtt / nb_paths;
}
