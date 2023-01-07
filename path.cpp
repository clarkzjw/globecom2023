//
// Created by clarkzjw on 12/19/22.
//

#include "tplayer.h"

vector<double> path_rtt[nb_paths];
vector<double> path_throughput[nb_paths];

vector<struct reward_item> history_rewards;


int previous_number = 2;

double get_latest_total_throughput() {
    double throughput = 0;
    for (auto & path : path_throughput) {
        if (!path.empty()) {
            throughput += path[path.size() - 1];
        }
    }
    return throughput;
}

double get_previous_average_throughput(int path_id) {
    double throughput = 0;
    int count = 0;
    for (int i = path_throughput[path_id].size() - 1; i >= 0; i--) {
        if (path_throughput[path_id][i] > 0) {
            throughput += path_throughput[path_id][i];
            count++;
        }
        if (count == 5) {
            break;
        }
    }
    if (path_throughput[path_id].empty() || count == 0) {
        return throughput;
    }
    return throughput / count;
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

double get_previous_average_rtt(int path_id, int nb_segments) {
    double rtt = 0;
    int count = 0;
    for (int i = path_rtt[path_id].size() - 1; i >= 0; i--) {
        if (path_rtt[path_id][i] > 0) {
            rtt += path_rtt[path_id][i];
            count++;
        }
        if (count == nb_segments) {
            break;
        }
    }
    if (path_rtt[path_id].empty() || count == 0) {
        return rtt;
    }
    return rtt / count;
}
