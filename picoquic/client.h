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
};

int quic_client(const char* ip_address_text, int server_port,
    picoquic_quic_config_t* config, int force_migration,
    int nb_packets_before_key_update, char const* client_scenario_text, char* if_name, struct picoquic_download_stat* stat);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_CLIENT_H
