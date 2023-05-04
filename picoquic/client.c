/*
 * Author: Christian Huitema
 * Copyright (c) 2017, Private Octopus, Inc.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Private Octopus, Inc. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "getopt.h"
#include <WinSock2.h>
#include <Windows.h>

#define SERVER_CERT_FILE "certs\\cert.pem"
#define SERVER_KEY_FILE "certs\\key.pem"

#else /* Linux */

#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>

#define SERVER_CERT_FILE "certs/cert.pem"
#define SERVER_KEY_FILE "certs/key.pem"

#endif

static const int default_server_port = 4443;
static const char* default_server_name = "::";
static const char* ticket_store_filename = "demo_ticket_store.bin";
static const char* token_store_filename = "demo_token_store.bin";

#include "h3zero.h"
#include "picosplay.h"

#include "autoqlog.h"
#include "client.h"
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
 * This is pretty complex, because the demo client is used to test a variety of interop
 * scenarios, for example:
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

char* picoquic_strsep(char** stringp, const char* delim)
{
#ifdef _WINDOWS
    if (*stringp == NULL) {
        return NULL;
    }
    char* token_start = *stringp;
    *stringp = strpbrk(token_start, delim);
    if (*stringp) {
        **stringp = '\0';
        (*stringp)++;
    }
    return token_start;
#else
    return strsep(stringp, delim);
#endif
}

/*
 * mp_config is consisted with src_if and alt_ip, seperated by "/" and ","
 * e.g. 10.0.0.2/3,10.0.0.3/4,10.0.0.4/5
 * where src_if is an int list, and alt_ip is a string list.
 * alt_ip is the ip of the alternative path
 * src_if is the index of the interface where the alt_ip is bounded with
 */
int picoquic_parse_client_multipath_config(char* mp_config, int* src_if, struct sockaddr_storage* alt_ip, int* nb_alt_paths)
{
    int ret = 0;
    int valid_ip, valid_index = 0;
    char *token, *token2, *ptr, *str;
    str = malloc(sizeof(char) * (strlen(mp_config) + 1));
    if (str == NULL) {
        ret = -1;
    }
    memcpy(str, mp_config, sizeof(char) * (strlen(mp_config) + 1));
    ptr = str;

    while ((token = picoquic_strsep(&str, ","))) {
        struct sockaddr_storage ip;
        valid_index = valid_ip = 0;

        while ((token2 = picoquic_strsep(&token, "/"))) {
            if (picoquic_store_text_addr(&ip, token2, 0) == 0) {
                memcpy(alt_ip + (*nb_alt_paths), &ip, sizeof(struct sockaddr_storage));
                valid_ip = 1;
            }
            if (atoi(token2) > 0) {
                *(src_if + (*nb_alt_paths)) = atoi(token2);
                valid_index = 1;
            }
        }

        if ((valid_ip == 1) && (valid_index == 1)) {
            (*nb_alt_paths)++;
            /* If more than PICOQUIC_NB_PATH_TARGET alt paths are specified, the remaining are ignored */
            if (*nb_alt_paths >= PICOQUIC_NB_PATH_TARGET) {
                break;
            }
        }
    }
    free(ptr);
    return ret;
}

int client_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
    void* callback_ctx, void* callback_arg)
{
    int ret = 0;
    client_loop_cb_t* cb_ctx = (client_loop_cb_t*)callback_ctx;

    if (cb_ctx == NULL) {
        ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
    } else {
        switch (cb_mode) {
        case picoquic_packet_loop_ready:
            fprintf(stdout, "Waiting for packets.\n");
            break;
        case picoquic_packet_loop_after_receive:
            /* Post receive callback */
            if ((!cb_ctx->is_siduck && !cb_ctx->is_quicperf && cb_ctx->demo_callback_ctx->connection_closed) || cb_ctx->cnx_client->cnx_state == picoquic_state_disconnected) {
                fprintf(stdout, "The connection is closed!\n");
                ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
                break;
            }
            /* Keeping track of the addresses and ports, as we
             * need them to verify the migration behavior */
            if (!cb_ctx->address_updated && cb_ctx->cnx_client->path[0]->local_addr.ss_family != 0) {
                uint16_t updated_port = (cb_ctx->cnx_client->path[0]->local_addr.ss_family == AF_INET) ? ((struct sockaddr_in*)&cb_ctx->cnx_client->path[0]->local_addr)->sin_port : ((struct sockaddr_in6*)&cb_ctx->cnx_client->path[0]->local_addr)->sin6_port;
                if (updated_port != 0) {
                    cb_ctx->address_updated = 1;
                    picoquic_store_addr(&cb_ctx->client_address, (struct sockaddr*)&cb_ctx->cnx_client->path[0]->local_addr);
                    fprintf(stdout, "Client port (AF=%d): %d.\n", cb_ctx->client_address.ss_family, updated_port);
                }
            }
            if (picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_client_almost_ready && cb_ctx->notified_ready == 0) {
                /* if almost ready, display results of negotiation */
                if (picoquic_tls_is_psk_handshake(cb_ctx->cnx_client)) {
                    fprintf(stdout, "The session was properly resumed!\n");
                    picoquic_log_app_message(cb_ctx->cnx_client,
                        "%s", "The session was properly resumed!");
                }

                if (cb_ctx->cnx_client->zero_rtt_data_accepted) {
                    fprintf(stdout, "Zero RTT data is accepted!\n");
                    picoquic_log_app_message(cb_ctx->cnx_client,
                        "%s", "Zero RTT data is accepted!");
                }

                if (cb_ctx->cnx_client->alpn != NULL) {
                    fprintf(stdout, "Negotiated ALPN: %s\n", cb_ctx->cnx_client->alpn);
                    picoquic_log_app_message(cb_ctx->cnx_client,
                        "Negotiated ALPN: %s", cb_ctx->cnx_client->alpn);
                    cb_ctx->saved_alpn = picoquic_string_duplicate(cb_ctx->cnx_client->alpn);
                }
                fprintf(stdout, "Almost ready!\n\n");
                cb_ctx->notified_ready = 1;
            } else if (ret == 0 && (picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_ready || picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_client_ready_start)) {

                if (picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_ready && cb_ctx->multipath_probe_done == 0) {
                    for (int i = 0; i < cb_ctx->nb_alt_paths; i++) {
                        if ((ret = picoquic_probe_new_path_ex(cb_ctx->cnx_client, (struct sockaddr*)&cb_ctx->server_address,
                                 (struct sockaddr*)&cb_ctx->client_alt_address[i], cb_ctx->client_alt_if[i], picoquic_get_quic_time(quic), 0))
                            != 0) {
                            picoquic_log_app_message(cb_ctx->cnx_client, "Probe new path failed with exit code %d\n", ret);
                        } else {
                            picoquic_log_app_message(cb_ctx->cnx_client, "New path added, total path available %d\n", cb_ctx->cnx_client->nb_paths);
                        }
                        cb_ctx->multipath_probe_done = 1;
                    }
                }

                /* Track the migration to server preferred address */
                if (cb_ctx->cnx_client->remote_parameters.prefered_address.is_defined && !cb_ctx->migration_to_preferred_finished) {
                    if (picoquic_compare_addr(
                            (struct sockaddr*)&cb_ctx->server_address, (struct sockaddr*)&cb_ctx->cnx_client->path[0]->peer_addr)
                        != 0) {
                        fprintf(stdout, "Migrated to server preferred address!\n");
                        picoquic_log_app_message(cb_ctx->cnx_client, "%s", "Migrated to server preferred address!");
                        cb_ctx->migration_to_preferred_finished = 1;
                    } else if (cb_ctx->cnx_client->nb_paths > 1 && !cb_ctx->migration_to_preferred_started) {
                        cb_ctx->migration_to_preferred_started = 1;
                        fprintf(stdout, "Attempting migration to server preferred address.\n");
                        picoquic_log_app_message(cb_ctx->cnx_client, "%s", "Attempting migration to server preferred address.");

                    } else if (cb_ctx->cnx_client->nb_paths == 1 && cb_ctx->migration_to_preferred_started) {
                        fprintf(stdout, "Could not migrate to server preferred address!\n");
                        picoquic_log_app_message(cb_ctx->cnx_client, "%s", "Could not migrate to server preferred address!");
                        cb_ctx->migration_to_preferred_finished = 1;
                    }
                }

                /* Execute the migration trials
                 * The actual change of sockets is delegated to the packet loop function,
                 * so it can be integrated with other aspects of socket management.
                 * If a new socket is needed, two special error codes will be used.
                 */
                if (cb_ctx->force_migration && cb_ctx->migration_started == 0 && cb_ctx->address_updated && picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_ready && (cb_ctx->cnx_client->cnxid_stash_first != NULL || cb_ctx->force_migration == 1) && (!cb_ctx->cnx_client->remote_parameters.prefered_address.is_defined || cb_ctx->migration_to_preferred_finished)) {
                    int mig_ret = 0;
                    cb_ctx->migration_started = 1;
                    cb_ctx->server_cid_before_migration = cb_ctx->cnx_client->path[0]->p_remote_cnxid->cnx_id;
                    if (cb_ctx->cnx_client->path[0]->p_local_cnxid != NULL) {
                        cb_ctx->client_cid_before_migration = cb_ctx->cnx_client->path[0]->p_local_cnxid->cnx_id;
                    } else {
                        /* Special case of forced migration after preferred address migration */
                        memset(&cb_ctx->client_cid_before_migration, 0, sizeof(picoquic_connection_id_t));
                    }
                    switch (cb_ctx->force_migration) {
                    case 1:
                        fprintf(stdout, "Switch to new port. Will test NAT rebinding support.\n");
                        ret = PICOQUIC_NO_ERROR_SIMULATE_NAT;
                        break;
                    case 2:
                        mig_ret = picoquic_renew_connection_id(cb_ctx->cnx_client, 0);
                        if (mig_ret != 0) {
                            if (mig_ret == PICOQUIC_ERROR_MIGRATION_DISABLED) {
                                fprintf(stdout, "Migration disabled, cannot test CNXID renewal.\n");
                            } else {
                                fprintf(stdout, "Renew CNXID failed, error: %x.\n", mig_ret);
                            }
                            cb_ctx->migration_started = -1;
                        } else {
                            fprintf(stdout, "Switching to new CNXID.\n");
                        }
                        break;
                    case 3:
                        fprintf(stdout, "Will test migration to new port.\n");
                        ret = PICOQUIC_NO_ERROR_SIMULATE_MIGRATION;
                        break;
                    default:
                        cb_ctx->migration_started = -1;
                        fprintf(stdout, "Invalid migration code: %d!\n", cb_ctx->force_migration);
                        break;
                    }
                }

                /* Track key update */
                if (cb_ctx->nb_packets_before_key_update > 0 && !cb_ctx->key_update_done && picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_ready && cb_ctx->cnx_client->nb_packets_received > (uint64_t)cb_ctx->nb_packets_before_key_update) {
                    int key_rot_ret = picoquic_start_key_rotation(cb_ctx->cnx_client);
                    if (key_rot_ret != 0) {
                        fprintf(stdout, "Will not test key rotation.\n");
                        picoquic_log_app_message(cb_ctx->cnx_client, "%s", "Will not test key rotation.");
                        cb_ctx->key_update_done = -1;
                    } else {
                        fprintf(stdout, "Key rotation started.\n");
                        picoquic_log_app_message(cb_ctx->cnx_client, "%s", "Key rotation started.");
                        cb_ctx->key_update_done = 1;
                    }
                }

                if (!cb_ctx->is_siduck && !cb_ctx->is_quicperf && cb_ctx->demo_callback_ctx->nb_open_streams == 0) {
                    fprintf(stdout, "All done, Closing the connection.\n");
                    picoquic_log_app_message(cb_ctx->cnx_client, "%s", "All done, Closing the connection.");

                    ret = picoquic_close(cb_ctx->cnx_client, 0);
                }
            }
            break;
        case picoquic_packet_loop_after_send:
            if (picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_disconnected) {
                ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
            } else if (ret == 0 && cb_ctx->established == 0 && (picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_ready || picoquic_get_cnx_state(cb_ctx->cnx_client) == picoquic_state_client_ready_start)) {
                printf("Connection established. Version = %x, I-CID: %llx, verified: %d\n",
                    picoquic_supported_versions[cb_ctx->cnx_client->version_index].version,
                    (unsigned long long)picoquic_val64_connection_id(picoquic_get_logging_cnxid(cb_ctx->cnx_client)),
                    cb_ctx->cnx_client->is_hcid_verified);

                picoquic_log_app_message(cb_ctx->cnx_client,
                    "Connection established. Version = %x, I-CID: %llx, verified: %d",
                    picoquic_supported_versions[cb_ctx->cnx_client->version_index].version,
                    (unsigned long long)picoquic_val64_connection_id(picoquic_get_logging_cnxid(cb_ctx->cnx_client)),
                    cb_ctx->cnx_client->is_hcid_verified);
                cb_ctx->established = 1;

                if (!cb_ctx->zero_rtt_available && !cb_ctx->is_siduck && !cb_ctx->is_quicperf) {
                    /* Start the download scenario */
                    ret = picoquic_demo_client_start_streams(cb_ctx->cnx_client, cb_ctx->demo_callback_ctx, PICOQUIC_DEMO_STREAM_ID_INITIAL);
                }
            }
            break;
        case picoquic_packet_loop_port_update:
            break;
        default:
            ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
            break;
        }
    }
    return ret;
}

/* Quic Client */
int quic_client(const char* ip_address_text, int server_port,
    picoquic_quic_config_t* config, int force_migration,
    int nb_packets_before_key_update, char const* client_scenario_text,
    const char* if_name, struct picoquic_download_stat* stat)
{
    /* Start: start the QUIC process with cert and key files */
    int ret = 0;
    picoquic_quic_t* qclient = NULL;
    picoquic_cnx_t* cnx_client = NULL;
    picoquic_demo_callback_ctx_t callback_ctx = { 0 };
    uint64_t current_time = 0;
    int is_name = 0;
    size_t client_sc_nb = 0;
    picoquic_demo_stream_desc_t* client_sc = NULL;
    int is_siduck = 0;
    siduck_ctx_t* siduck_ctx = NULL;
    int is_quicperf = 0;
    quicperf_ctx_t* quicperf_ctx = NULL;
    client_loop_cb_t loop_cb;
    const char* sni = config->sni;

    memset(&loop_cb, 0, sizeof(client_loop_cb_t));

    if (ret == 0) {
        ret = picoquic_get_server_address(ip_address_text, server_port, &loop_cb.server_address, &is_name);
        if (sni == NULL && is_name != 0) {
            sni = ip_address_text;
        }
    }

    /* Create QUIC context */
    current_time = picoquic_current_time();
    callback_ctx.last_interaction_time = current_time;

    if (ret == 0) {
        if (config->ticket_file_name == NULL) {
            ret = picoquic_config_set_option(config, picoquic_option_Ticket_File_Name, ticket_store_filename);
        }
        if (ret == 0 && config->token_file_name == NULL) {
            ret = picoquic_config_set_option(config, picoquic_option_Token_File_Name, token_store_filename);
        }
        if (ret == 0) {
            qclient = picoquic_create_and_configure(config, NULL, NULL, current_time, NULL);
            if (qclient == NULL) {
                ret = -1;
            } else {
                picoquic_set_key_log_file_from_env(qclient);

                if (config->qlog_dir != NULL) {
                    picoquic_set_qlog(qclient, config->qlog_dir);
                }

                if (config->performance_log != NULL) {
                    ret = picoquic_perflog_setup(qclient, config->performance_log);
                }
            }
        }
    }

    /* If needed, set ALPN and proposed version from tickets */
    if (ret == 0) {
        if (config->alpn == NULL || config->proposed_version == 0) {
            char const* ticket_alpn;
            uint32_t ticket_version;

            if (picoquic_demo_client_get_alpn_and_version_from_tickets(qclient, sni, config->alpn,
                    config->proposed_version, current_time, &ticket_alpn, &ticket_version)
                == 0) {
                if (ticket_alpn != NULL) {
                    fprintf(stdout, "Set ALPN to %s based on stored ticket\n", ticket_alpn);
                    picoquic_config_set_option(config, picoquic_option_ALPN, ticket_alpn);
                }

                if (ticket_version != 0) {
                    fprintf(stdout, "Set version to 0x%08x based on stored ticket\n", ticket_version);
                    config->proposed_version = ticket_version;
                }
            }
        }
    }

    if (ret == 0) {
        if (config->alpn != NULL && (strcmp(config->alpn, "siduck") == 0 || strcmp(config->alpn, "siduck-00") == 0)) {
            /* Set a siduck client */
            is_siduck = 1;
            siduck_ctx = siduck_create_ctx(stdout);
            if (siduck_ctx == NULL) {
                fprintf(stdout, "Could not get ready to quack\n");
                return -1;
            }
            fprintf(stdout, "Getting ready to quack\n");
        } else if (config->alpn != NULL && strcmp(config->alpn, QUICPERF_ALPN) == 0) {
            /* Set a QUICPERF client */
            is_quicperf = 1;
            quicperf_ctx = quicperf_create_ctx(client_scenario_text);
            if (quicperf_ctx == NULL) {
                fprintf(stdout, "Could not get ready to run QUICPERF\n");
                return -1;
            }
            fprintf(stdout, "Getting ready to run QUICPERF\n");
        } else {
            if (config->no_disk) {
                fprintf(stdout, "Files not saved to disk (-D, no_disk)\n");
            }

            if (client_scenario_text == NULL) {
                client_scenario_text = "index.html";
            }

            fprintf(stdout, "Testing scenario: <%s>\n", client_scenario_text);
            ret = demo_client_parse_scenario_desc(client_scenario_text, &client_sc_nb, &client_sc);
            if (ret != 0) {
                fprintf(stdout, "Cannot parse the specified scenario.\n");
                return -1;
            } else {
                ret = picoquic_demo_client_initialize_context(&callback_ctx, client_sc, client_sc_nb, config->alpn, config->no_disk, 0);
                callback_ctx.out_dir = config->out_dir;
            }
        }
    }
    /* Check that if we are using H3 the SNI is not NULL */
    if (ret == 0 && sni == NULL) {
        fprintf(stdout, "Careful: NULL SNI is incompatible with HTTP 3. Expect errors!\n");
    }

    /* Create the client connection */
    if (ret == 0) {
        /* Create a client connection */
        cnx_client = picoquic_create_cnx(qclient, picoquic_null_connection_id, picoquic_null_connection_id,
            (struct sockaddr*)&loop_cb.server_address, current_time,
            config->proposed_version, sni, config->alpn, 1);

        if (cnx_client == NULL) {
            ret = -1;
        } else {
            /* Set PMTUD policy to delayed on the client, leave to default=basic on server */
            picoquic_cnx_set_pmtud_policy(cnx_client, picoquic_pmtud_delayed);
            picoquic_set_default_pmtud_policy(qclient, picoquic_pmtud_delayed);

            if (is_siduck) {
                picoquic_set_callback(cnx_client, siduck_callback, siduck_ctx);
                cnx_client->local_parameters.max_datagram_frame_size = 128;
            } else if (is_quicperf) {
                picoquic_set_callback(cnx_client, quicperf_callback, quicperf_ctx);
            } else {
                picoquic_set_callback(cnx_client, picoquic_demo_client_callback, &callback_ctx);

                /* Requires TP grease, for interop tests */
                cnx_client->grease_transport_parameters = 1;
                cnx_client->local_parameters.enable_time_stamp = 3;
                cnx_client->local_parameters.do_grease_quic_bit = 1;

                if (callback_ctx.tp != NULL) {
                    picoquic_set_transport_parameters(cnx_client, callback_ctx.tp);
                }
            }

            if (config->large_client_hello) {
                cnx_client->test_large_chello = 1;
            }

//            if (config->esni_rr_file != NULL) {
//                ret = picoquic_esni_client_from_file(cnx_client, config->esni_rr_file);
//            }

            if (config->desired_version != 0) {
                picoquic_set_desired_version(cnx_client, config->desired_version);
            }

            fprintf(stdout, "Max stream id bidir remote before start = %d (%d)\n",
                (int)cnx_client->max_stream_id_bidir_remote,
                (int)cnx_client->remote_parameters.initial_max_stream_id_bidir);

            if (ret == 0) {
                ret = picoquic_start_client_cnx(cnx_client);

                printf("Starting client connection. Version = %x, I-CID: %llx\n",
                    picoquic_supported_versions[cnx_client->version_index].version,
                    (unsigned long long)picoquic_val64_connection_id(picoquic_get_logging_cnxid(cnx_client)));

                fprintf(stdout, "Max stream id bidir remote after start = %d (%d)\n",
                    (int)cnx_client->max_stream_id_bidir_remote,
                    (int)cnx_client->remote_parameters.initial_max_stream_id_bidir);
            }

            if (ret == 0 && !is_siduck && !is_quicperf) {
                if (picoquic_is_0rtt_available(cnx_client) && (config->proposed_version & 0x0a0a0a0a) != 0x0a0a0a0a) {
                    loop_cb.zero_rtt_available = 1;

                    fprintf(stdout, "Max stream id bidir remote after 0rtt = %d (%d)\n",
                        (int)cnx_client->max_stream_id_bidir_remote,
                        (int)cnx_client->remote_parameters.initial_max_stream_id_bidir);

                    /* Queue a simple frame to perform 0-RTT test */
                    /* Start the download scenario */

                    ret = picoquic_demo_client_start_streams(cnx_client, &callback_ctx, PICOQUIC_DEMO_STREAM_ID_INITIAL);
                }
            }
        }
    }

    /* Wait for packets */
    if (ret == 0) {
        if (config->multipath_alt_config != NULL) {
            picoquic_parse_client_multipath_config(config->multipath_alt_config, loop_cb.client_alt_if, loop_cb.client_alt_address, &loop_cb.nb_alt_paths);
        }

        loop_cb.cnx_client = cnx_client;
        loop_cb.force_migration = force_migration;
        loop_cb.nb_packets_before_key_update = nb_packets_before_key_update;
        loop_cb.is_siduck = is_siduck;
        loop_cb.is_quicperf = is_quicperf;
        loop_cb.socket_buffer_size = config->socket_buffer_size;
        loop_cb.demo_callback_ctx = &callback_ctx;

        if (strlen(if_name) > 0) {
            ret = picoquic_packet_loop_bind_interface(qclient, 0, loop_cb.server_address.ss_family, 0,
                config->socket_buffer_size, config->do_not_use_gso, client_loop_cb, &loop_cb, if_name);
        } else {
            ret = picoquic_packet_loop(qclient, 0, loop_cb.server_address.ss_family, 0,
                config->socket_buffer_size, config->do_not_use_gso, client_loop_cb, &loop_cb);
        }
    }

    if (ret == 0) {
        uint64_t last_err;

        if ((last_err = picoquic_get_local_error(cnx_client)) != 0) {
            fprintf(stdout, "Connection end with local error 0x%" PRIx64 ".\n", last_err);
            ret = -1;
        }
        if ((last_err = picoquic_get_remote_error(cnx_client)) != 0) {
            fprintf(stdout, "Connection end with remote error 0x%" PRIx64 ".\n", last_err);
            ret = -1;
        }
        if ((last_err = picoquic_get_application_error(cnx_client)) != 0) {
            fprintf(stdout, "Connection end with application error 0x%" PRIx64 ".\n", last_err);
            ret = -1;
        }

        /* Report on successes and failures */
        if (cnx_client->nb_zero_rtt_sent != 0) {
            fprintf(stdout, "Out of %d zero RTT packets, %d were acked by the server.\n",
                cnx_client->nb_zero_rtt_sent, cnx_client->nb_zero_rtt_acked);
            picoquic_log_app_message(cnx_client, "Out of %d zero RTT packets, %d were acked by the server.",
                cnx_client->nb_zero_rtt_sent, cnx_client->nb_zero_rtt_acked);
        }

        fprintf(stdout, "Quic Bit was %sgreased by the client.\n", (cnx_client->quic_bit_greased) ? "" : "NOT ");
        fprintf(stdout, "Quic Bit was %sgreased by the server.\n", (cnx_client->quic_bit_received_0) ? "" : "NOT ");

        if (cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ect0_total_local != 0 || cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ect1_total_local != 0 || cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ce_total_local != 0) {
            fprintf(stdout, "ECN was received (ect0: %" PRIu64 ", ect1: %" PRIu64 ", ce: %" PRIu64 ").\n",
                cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ect0_total_local,
                cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ect1_total_local,
                cnx_client->ack_ctx[picoquic_packet_context_application].ecn_ce_total_local);
        } else {
            fprintf(stdout, "ECN was not received.\n");
        }

        if (cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ect0_total_remote != 0 || cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ect1_total_remote != 0 || cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ce_total_remote != 0) {
            fprintf(stdout, "ECN was acknowledged (ect0: %" PRIu64 ", ect1: %" PRIu64 ", ce: %" PRIu64 ").\n",
                cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ect0_total_remote,
                cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ect1_total_remote,
                cnx_client->pkt_ctx[picoquic_packet_context_application].ecn_ce_total_remote);
        } else {
            fprintf(stdout, "ECN was not acknowledged.\n");
        }

        if (config->desired_version != 0) {
            uint32_t v = picoquic_supported_versions[cnx_client->version_index].version;
            if (v == config->desired_version) {
                fprintf(stdout, "Successfully negotiated version 0x%" PRIx32 ".\n", v);
            } else {
                fprintf(stdout, "Could not negotiate version 0x%" PRIx32 ", used version 0x%" PRIu32 ".\n",
                    config->desired_version, v);
            }
        }

        if (loop_cb.force_migration) {
            if (!loop_cb.migration_started) {
                fprintf(stdout, "Could not start testing migration.\n");
                picoquic_log_app_message(cnx_client, "%s", "Could not start testing migration.");
                loop_cb.migration_started = -1;
            } else {
                int source_addr_cmp = picoquic_compare_addr(
                    (struct sockaddr*)&cnx_client->path[0]->local_addr,
                    (struct sockaddr*)&loop_cb.client_address);
                int dest_cid_cmp = picoquic_compare_connection_id(
                    &cnx_client->path[0]->p_remote_cnxid->cnx_id,
                    &loop_cb.server_cid_before_migration);
                fprintf(stdout, "After migration:\n");
                fprintf(stdout, "- Default source address %s\n", (source_addr_cmp) ? "changed" : "did not change");
                if (cnx_client->path[0]->p_local_cnxid == NULL) {
                    fprintf(stdout, "- Local CID is NULL!\n");
                } else {
                    int source_cid_cmp = picoquic_compare_connection_id(
                        &cnx_client->path[0]->p_local_cnxid->cnx_id,
                        &loop_cb.client_cid_before_migration);
                    fprintf(stdout, "- Local CID %s\n", (source_cid_cmp) ? "changed" : "did not change");
                }
                fprintf(stdout, "- Remode CID %s\n", (dest_cid_cmp) ? "changed" : "did not change");
            }
        }

        if (loop_cb.nb_packets_before_key_update > 0) {
            if (loop_cb.key_update_done == 0) {
                fprintf(stdout, "Did not start key rotation.\n");
            } else if (loop_cb.key_update_done == -1) {
                fprintf(stdout, "Error when starting key rotation.\n");
            } else {
                uint64_t crypto_rotation_sequence;
                if (cnx_client->is_multipath_enabled) {
                    crypto_rotation_sequence = cnx_client->path[0]->p_local_cnxid->ack_ctx.crypto_rotation_sequence;
                } else {
                    crypto_rotation_sequence = cnx_client->ack_ctx[picoquic_packet_context_application].crypto_rotation_sequence;
                }
                fprintf(stdout, "Crypto rotation sequence: %" PRIu64 ", phase ENC: %d, phase DEC: %d\n",
                    crypto_rotation_sequence, cnx_client->key_phase_enc, cnx_client->key_phase_dec);
            }
        }

        if (picoquic_get_data_received(cnx_client) > 0) {
            uint64_t start_time = picoquic_get_cnx_start_time(cnx_client);
            uint64_t close_time = picoquic_get_quic_time(qclient);
            double duration_usec = (double)(close_time - start_time);

            if (duration_usec > 0) {
                double receive_rate_mbps = 8.0 * ((double)picoquic_get_data_received(cnx_client)) / duration_usec;
                fprintf(stdout, "Received %s %" PRIu64 " bytes in %f seconds, %f Mbps.\n",
                        client_scenario_text,
                        picoquic_get_data_received(cnx_client),
                        duration_usec / 1000000.0, receive_rate_mbps);
                picoquic_log_app_message(cnx_client, "Received %" PRIu64 " bytes in %f seconds, %f Mbps.",
                                         picoquic_get_data_received(cnx_client),
                                         duration_usec / 1000000.0, receive_rate_mbps);

                if (stat != NULL) {
                    stat->time = duration_usec / 1000000.0;
                    stat->throughput = receive_rate_mbps;
                    stat->rtt = picoquic_get_rtt(cnx_client);
                    stat->one_way_delay_avg = picoquic_get_delay(cnx_client);
                    stat->bandwidth_estimate = picoquic_get_bandwidth_estimate(cnx_client);
                    stat->total_bytes_lost = picoquic_get_total_bytes_lost(cnx_client);
                    stat->total_received = picoquic_get_total_received(cnx_client);
                    stat->data_received = picoquic_get_data_received(cnx_client);
                }
#if 0
                /* Print those for debugging the effects of ack frequency and flow control */
                printf("max_data_local: %" PRIu64 "\n", cnx_client->maxdata_local);
                printf("max_max_stream_data_local: %" PRIu64 "\n", cnx_client->max_max_stream_data_local);
                printf("max_data_remote: %" PRIu64 "\n", cnx_client->maxdata_remote);
                printf("max_max_stream_data_remote: %" PRIu64 "\n", cnx_client->max_max_stream_data_remote);
                printf("ack_delay_remote: %" PRIu64 " ... %" PRIu64 "\n",
                       cnx_client->min_ack_delay_remote, cnx_client->max_ack_delay_remote);
                printf("max_ack_gap_remote: %" PRIu64 "\n", cnx_client->max_ack_gap_remote);
                printf("ack_delay_local: %" PRIu64 " ... %" PRIu64 "\n",
                       cnx_client->min_ack_delay_local, cnx_client->max_ack_delay_local);
                printf("max_ack_gap_local: %" PRIu64 "\n", cnx_client->max_ack_gap_local);
                printf("max_mtu_sent: %zu\n", cnx_client->max_mtu_sent);
                printf("max_mtu_received: %zu\n", cnx_client->max_mtu_received);
#endif
            }
        }
    }

    if (qclient != NULL) {
        uint8_t* ticket;
        uint16_t ticket_length;

        if (sni != NULL && loop_cb.saved_alpn != NULL && 0 == picoquic_get_ticket(qclient->p_first_ticket, current_time, sni, (uint16_t)strlen(sni), loop_cb.saved_alpn, (uint16_t)strlen(loop_cb.saved_alpn), 0, &ticket, &ticket_length, NULL, 0)) {
            fprintf(stdout, "Received ticket from %s (%s):\n", sni, loop_cb.saved_alpn);
            picoquic_log_picotls_ticket(stdout, picoquic_null_connection_id, ticket, ticket_length);
        }

        if (picoquic_save_session_tickets(qclient, config->ticket_file_name) != 0) {
            fprintf(stderr, "Could not store the saved session tickets to <%s>.\n", config->ticket_file_name);
        }

        if (picoquic_save_retry_tokens(qclient, config->token_file_name) != 0) {
            fprintf(stderr, "Could not save tokens to <%s>.\n", config->token_file_name);
        }

        picoquic_free(qclient);
    }

    /* Clean up */
    if (is_quicperf) {
        if (quicperf_ctx != NULL) {
            quicperf_delete_ctx(quicperf_ctx);
        }
    } else if (is_siduck) {
        if (siduck_ctx != NULL) {
            free(siduck_ctx);
        }
    } else {
        picoquic_demo_client_delete_context(&callback_ctx);
    }

    if (loop_cb.saved_alpn != NULL) {
        free((void*)loop_cb.saved_alpn);
        loop_cb.saved_alpn = NULL;
    }

    if (client_scenario_text != NULL && client_sc != NULL) {
        demo_client_delete_scenario_desc(client_sc_nb, client_sc);
        client_sc = NULL;
    }
    return ret;
}
