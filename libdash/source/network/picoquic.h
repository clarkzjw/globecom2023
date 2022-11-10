//
// Created by Jinwei Zhao on 2022-06-16.
//

#ifndef PLAYER_PICOQUIC_H
#define PLAYER_PICOQUIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <netinet/in.h>

//static const int default_server_port = 4443;
//static const char* default_server_name = "::";
static const char* ticket_store_filename = "demo_ticket_store.bin";
static const char* token_store_filename = "demo_token_store.bin";

#include "autoqlog.h"
#include "h3zero.h"
// h3zero.h should be included before democlient.h
#include "democlient.h"
#include "performance_log.h"
#include "picoquic.h"
#include "picoquic_config.h"
#include "picoquic_internal.h"
#include "picoquic_logger.h"
#include "picoquic_packet_loop.h"
#include "picoquic_unified_log.h"
#include "picoquic_utils.h"
#include "quicperf.h"
#include "siduck.h"

/* Client loop call back management.
 * This is pretty complex, because the demo client is used to test a variety of
 * interop scenarios, for example:
 *
 * Variants of migration:
 * - Basic NAT traversal (1)
 * - Simple CID swap (2)
 * - Organized migration (3)
 * Encryption key rotation, after some number of packets have been sent.
 *
 * The client loop terminates when the client connection is closed.
 */

typedef struct st_client_loop_cb_t {
    picoquic_cnx_t* cnx_client;
    picoquic_demo_callback_ctx_t* demo_callback_ctx;
    siduck_ctx_t* siduck_ctx;
    int notified_ready;
    int established;
    int migration_to_preferred_started;
    int migration_to_preferred_finished;
    int migration_started;
    int address_updated;
    int force_migration;
    int nb_packets_before_key_update;
    int key_update_done;
    int zero_rtt_available;
    int is_siduck;
    int is_quicperf;
    int socket_buffer_size;
    int multipath_probe_done;
    char const* saved_alpn;
    struct sockaddr_storage server_address;
    struct sockaddr_storage client_address;
    struct sockaddr_storage client_alt_address[PICOQUIC_NB_PATH_TARGET];
    int client_alt_if[PICOQUIC_NB_PATH_TARGET];
    int nb_alt_paths;
    picoquic_connection_id_t server_cid_before_migration;
    picoquic_connection_id_t client_cid_before_migration;
} client_loop_cb_t;

int quic_client(const char* ip_address_text, int server_port,
                picoquic_quic_config_t* config, char const* client_scenario_text);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_PICOQUIC_H
