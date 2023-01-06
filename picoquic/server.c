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
#include "demoserver.h"
#include "performance_log.h"
#include "picoquic.h"
#include "picoquic_config.h"
#include "picoquic_internal.h"
#include "picoquic_lb.h"
#include "picoquic_packet_loop.h"
#include "picoquic_unified_log.h"
#include "picoquic_utils.h"

/* server loop call back management */
typedef struct st_server_loop_cb_t {
    int just_once;
    int first_connection_seen;
    int connection_done;
} server_loop_cb_t;

static int server_loop_cb(picoquic_quic_t* quic, picoquic_packet_loop_cb_enum cb_mode,
    void* callback_ctx, void* callback_arg)
{
    int ret = 0;
    server_loop_cb_t* cb_ctx = (server_loop_cb_t*)callback_ctx;

    if (cb_ctx == NULL) {
        ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
    } else {
        switch (cb_mode) {
        case picoquic_packet_loop_ready:
            fprintf(stdout, "Waiting for packets.\n");
            break;
        case picoquic_packet_loop_after_receive:
            break;
        case picoquic_packet_loop_after_send:
            break;
        case picoquic_packet_loop_port_update:
            break;
        default:
            ret = PICOQUIC_ERROR_UNEXPECTED_ERROR;
            break;
        }

        if (ret == 0 && cb_ctx->just_once) {
            if (!cb_ctx->first_connection_seen && picoquic_get_first_cnx(quic) != NULL) {
                cb_ctx->first_connection_seen = 1;
                fprintf(stdout, "First connection noticed.\n");
            } else if (cb_ctx->first_connection_seen && picoquic_get_first_cnx(quic) == NULL) {
                fprintf(stdout, "No more active connections.\n");
                cb_ctx->connection_done = 1;
                ret = PICOQUIC_NO_ERROR_TERMINATE_PACKET_LOOP;
            }
        }
    }
    return ret;
}

int quic_server(const char* server_name, picoquic_quic_config_t* config, int just_once)
{
    /* Start: start the QUIC process with cert and key files */
    int ret = 0;
    picoquic_quic_t* qserver = NULL;
    uint64_t current_time = 0;
    picohttp_server_parameters_t picoquic_file_param;
    server_loop_cb_t loop_cb_ctx;

    memset(&picoquic_file_param, 0, sizeof(picohttp_server_parameters_t));
    picoquic_file_param.web_folder = config->www_dir;
    memset(&loop_cb_ctx, 0, sizeof(server_loop_cb_t));
    loop_cb_ctx.just_once = just_once;

    /* Setup the server context */
    if (ret == 0) {
        current_time = picoquic_current_time();
        /* Create QUIC context */

        if (config->ticket_file_name == NULL) {
            ret = picoquic_config_set_option(config, picoquic_option_Ticket_File_Name, ticket_store_filename);
        }
        if (ret == 0 && config->token_file_name == NULL) {
            ret = picoquic_config_set_option(config, picoquic_option_Token_File_Name, token_store_filename);
        }
        if (ret == 0) {
            qserver = picoquic_create_and_configure(config, picoquic_demo_server_callback, &picoquic_file_param, current_time, NULL);
            if (qserver == NULL) {
                ret = -1;
            } else {
                picoquic_set_key_log_file_from_env(qserver);
                picoquic_set_default_priority(qserver, 4);

                picoquic_set_alpn_select_fn(qserver, picoquic_demo_server_callback_select_alpn);

                picoquic_set_mtu_max(qserver, config->mtu_max);

                picoquic_use_unique_log_names(qserver, 1);

                if (config->qlog_dir != NULL) {
                    picoquic_set_qlog(qserver, config->qlog_dir);
                }
                if (config->performance_log != NULL) {
                    ret = picoquic_perflog_setup(qserver, config->performance_log);
                }
                if (ret == 0 && config->cnx_id_cbdata != NULL) {
                    picoquic_load_balancer_config_t lb_config;
                    ret = picoquic_lb_compat_cid_config_parse(&lb_config, config->cnx_id_cbdata, strlen(config->cnx_id_cbdata));
                    if (ret != 0) {
                        fprintf(stdout, "Cannot parse the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
                    } else {
                        ret = picoquic_lb_compat_cid_config(qserver, &lb_config);
                        if (ret != 0) {
                            fprintf(stdout, "Cannot set the CNX_ID config policy: %s.\n", config->cnx_id_cbdata);
                        }
                    }
                }
            }
        }
    }

    if (ret == 0) {
        /* Wait for packets */
        ret = picoquic_packet_loop(qserver, config->server_port, 0, config->dest_if,
            config->socket_buffer_size, config->do_not_use_gso, server_loop_cb, &loop_cb_ctx);
    }

    /* And exit */
    printf("Server exit, ret = 0x%x\n", ret);

    /* Clean up */
    if (config->cnx_id_cbdata != NULL) {
        picoquic_lb_compat_cid_config_free(qserver);
    }
    if (qserver != NULL) {
        picoquic_free(qserver);
    }

    return ret;
}

/* TODO: rewrite using common code */
void usage()
{
    fprintf(stderr, "PicoQUIC demo client and server\n");
    fprintf(stderr, "Usage: picoquicdemo <options> [server_name [port [scenario]]] \n");
    fprintf(stderr, "  For the client mode, specify server_name and port.\n");
    fprintf(stderr, "  For the server mode, use -p to specify the port.\n");
    picoquic_config_usage();
    fprintf(stderr, "Picoquic demo options:\n");
    fprintf(stderr, "  -A ip/ifindex[,ip/ifindex]  IP and interface index for multipath alternative path, e.g. 10.0.0.2/3,10.0.0.3/4\n");
    fprintf(stderr, "  -f migration_mode     Force client to migrate to start migration:\n");
    fprintf(stderr, "                        -f 1  test NAT rebinding,\n");
    fprintf(stderr, "                        -f 2  test CNXID renewal,\n");
    fprintf(stderr, "                        -f 3  test migration to new address.\n");
    fprintf(stderr, "  -u nb                 trigger key update after receiving <nb> packets on client\n");
    fprintf(stderr, "  -1                    Once: close the server after processing 1 connection.\n");

    fprintf(stderr, "\nThe scenario argument specifies the set of files that should be retrieved,\n");
    fprintf(stderr, "and their order. The syntax is:\n");
    fprintf(stderr, "  *{[<stream_id>':'[<previous_stream>':'[<format>:]]]path;}\n");
    fprintf(stderr, "where:\n");
    fprintf(stderr, "  <stream_id>:          The numeric ID of the QUIC stream, e.g. 4. By default, the\n");
    fprintf(stderr, "                        next stream in the logical QUIC order, 0, 4, 8, etc.");
    fprintf(stderr, "  <previous_stream>:    The numeric ID of the previous stream. The GET command will\n");
    fprintf(stderr, "                        be issued after that stream's transfer finishes. By default,\n");
    fprintf(stderr, "                        previous stream in this scenario.\n");
    fprintf(stderr, "  <format>:             Whether the received file should be written to disc as\n");
    fprintf(stderr, "                        binary(b) or text(t). Defaults to text.\n");
    fprintf(stderr, "  <path>:               The name of the document that should be retrieved\n");
    fprintf(stderr, "If no scenario is specified, the client executes the default scenario.\n");

    exit(1);
}

int main(int argc, char** argv)
{
    picoquic_quic_config_t config;
    char option_string[512];
    int opt;
    const char* server_name = default_server_name;
    int server_port = default_server_port;
    char default_server_cert_file[512];
    char default_server_key_file[512];
    int just_once = 0;
    int ret;

    picoquic_config_init(&config);
    ret = picoquic_config_option_letters(option_string, sizeof(option_string), NULL);

    if (ret == 0) {
        /* Get the parameters */
        while ((opt = getopt(argc, argv, option_string)) != -1) {
            if (picoquic_config_command_line(opt, &optind, argc, (char const**)argv, optarg, &config) != 0) {
                usage();
            }
        }
    }

    if (optind < argc) {
        if ((server_port = atoi(argv[optind++])) <= 0) {
            fprintf(stderr, "Invalid port: %s\n", optarg);
            usage();
        }
    }

    if (optind < argc) {
        usage();
    }

    if (config.server_port == 0) {
        config.server_port = server_port;
    }

    if (config.server_cert_file == NULL && picoquic_get_input_path(default_server_cert_file, sizeof(default_server_cert_file), config.solution_dir, SERVER_CERT_FILE) == 0) {
        /* Using set option call to ensure proper memory management*/
        picoquic_config_set_option(&config, picoquic_option_CERT, default_server_cert_file);
    }

    if (config.server_key_file == NULL && picoquic_get_input_path(default_server_key_file, sizeof(default_server_key_file), config.solution_dir, SERVER_KEY_FILE) == 0) {
        /* Using set option call to ensure proper memory management*/
        picoquic_config_set_option(&config, picoquic_option_KEY, default_server_key_file);
    }

    /* Run as server */
    printf("Starting Picoquic server (v%s) on port %d, server name = %s, just_once = %d, do_retry = %d\n",
        PICOQUIC_VERSION, config.server_port, server_name, just_once, config.do_retry);
    ret = quic_server(server_name, &config, just_once);
    printf("Server exit with code = %d\n", ret);

    picoquic_config_clear(&config);
}
