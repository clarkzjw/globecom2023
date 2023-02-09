//
// Created by clarkzjw on 11/9/22.
//

#ifndef PLAYER_CLIENT_H
#define PLAYER_CLIENT_H

#include "picoquic_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct picoquic_download_stat {
    double time;
    double throughput;
    uint64_t rtt;
    uint64_t one_way_delay_avg;
    uint64_t bandwidth_estimate;
    uint64_t total_bytes_lost;
    uint64_t total_received;
    uint64_t data_received;
};

int quic_client(const char* ip_address_text, int server_port,
    picoquic_quic_config_t* config, int force_migration,
    int nb_packets_before_key_update, char const* client_scenario_text, const char* if_name, struct picoquic_download_stat* stat);


int example_download(char *filename);

#ifdef __cplusplus
}
#endif


#endif // PLAYER_CLIENT_H
