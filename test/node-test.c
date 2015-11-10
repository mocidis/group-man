#include "node.h"
#include <stdio.h>
#include "ansi-utils.h"
#include "gb-receiver.h"
#include "proto-constants.h"

#include "my-pjlib-utils.h"
/*
void on_adv_info(adv_server_t *adv_server, adv_request_t *request) {
    SHOW_LOG(4, fprintf(stdout,"Received: ID = %s\nSDP addr %s:%d\n", request->adv_info.adv_owner, request->adv_info.sdp_mip, request->adv_info.sdp_port));
}*/

void on_online_report(char *id, char *desc, int port, int is_online) {
    SHOW_LOG(5, fprintf(stdout, "O_REPT:%s(online=%d - port=%d)\n", id, is_online, port));
}
void on_tx_report(char *id, int is_tx) {
    SHOW_LOG(5, fprintf(stdout, "T_REPT:%s(tx=%d)\n", id, is_tx));
}
void on_rx_report(char *id, int is_rx) {
    SHOW_LOG(5, fprintf(stdout, "R_REPT:%s(rx=%d)\n", id, is_rx));
}
void on_sq_report(char *id, int is_sq) {
    SHOW_LOG(5, fprintf(stdout, "Q_REPT:%s(sq=%d)\n", id, is_sq));
}

static void init_adv_server(adv_server_t *adv_server, char *adv_cs, node_t *node) {
    memset(adv_server, 0, sizeof(*adv_server));

    adv_server->on_request_f = &on_adv_info;
    adv_server->on_open_socket_f = &on_open_socket_adv_server;
    adv_server->user_data = node;
    
    adv_server_init(adv_server, adv_cs);
    adv_server_start(adv_server);
}

void *auto_register(void *node_data) {
    node_t *node = (node_t *)node_data;
    while (1) {
        node_register(node);
        usleep(5*1000*1000);
    }
}

void usage(char *app) {
    printf("usage: %s <id> <location> <desc> <radio_port> <gm_cs> <gmc_cs> <guest> <streamer_spec> <receiver_spec>\n", app);
    printf("\tspec = file:path | dev:idx\n");
    exit(-1);
}

int main(int argc, char * argv[]) {
    node_t node;
    gb_receiver_t gr;

    char *gm_cs;
    char *gmc_cs;
    char adv_cs[30];
    char option[10];
    char gb_cs[30];

    char *streamer_spec;
    char *receiver_spec;
    char *first, *second;

    int adv_port = ADV_PORT;
    int gb_port = GB_PORT; 
    char *guest = argv[7];
    adv_server_t adv_server;
    int n;

    pthread_t thread;
    
    pj_caching_pool cp;
    pj_pool_t *pool = NULL;
    pjmedia_endpt *ep;
    endpoint_t streamer;
    endpoint_t receiver;
    
    if (argc < 10) usage(argv[0]);

    SET_LOG_LEVEL(4);

    gm_cs = argv[5];
    gmc_cs = argv[6];
    n = sprintf(adv_cs, "udp:0.0.0.0:%d", adv_port);
    adv_cs[n] = '\0';
    n = sprintf(gb_cs, "udp:%s:%d", GB_MIP, gb_port);
    gb_cs[n] = '\0';

    streamer_spec = argv[8];
    receiver_spec = argv[9];

    SHOW_LOG(5, fprintf(stdout, "%s - %s - %s - %s - %s - %s - %s - %s - %s - %s\n",argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], adv_cs, gb_cs, streamer_spec, receiver_spec));

    ///// Init node
    memset(&node, 0, sizeof(node));
    //node.on_adv_info_f = &on_adv_info;
    init_adv_server(&adv_server, adv_cs, &node);
    node_init(&node, argv[1], argv[2], argv[3], atoi(argv[4]), gm_cs, gmc_cs, adv_cs);
    node_add_adv_server(&node, &adv_server);

    /////// Init media endpoints
    pj_init();
    pj_log_set_level(3);
    pj_srand(1234);
    pj_caching_pool_init(&cp, NULL, 1024);
    pool = pj_pool_create(&cp.factory, "pool1", 1024, 1024, NULL);
    pjmedia_endpt_create(&cp.factory, NULL, 1, &ep);
    pjmedia_codec_g711_init(ep);

    streamer_init(&streamer, ep, pool);
    receiver_init(&receiver, ep, pool, 2);
    
    first = streamer_spec;
    second = strchr(streamer_spec, ':');
    *second = '\0';
    second++;
    int devidx = atoi(second);
    printf("second=%s devidx=%d\n", second, devidx);
    if( strcmp(first, "file") == 0 ) {
        SHOW_LOG(3, fprintf(stdout, "source: fileeeeeeeee\n"));
        streamer_config_file_source(&streamer, second);
    }
    else if( strcmp(first, "dev") == 0 ) {
        SHOW_LOG(3, fprintf(stdout, "source: deveeeeeeeee\n"));
        streamer_config_dev_source(&streamer, atoi(second));
    }
    else {
        EXIT_IF_TRUE(1, "wrong streamer spec\n");
    }

    first = receiver_spec;
    second = strchr(receiver_spec, ':');
    *second = '\0';
    second++;
    if( strcmp(first, "file") == 0 ) {
        SHOW_LOG(3, fprintf(stdout, "sink: fileeeeeeeee\n"));
        receiver_config_file_sink(&receiver, second);
    }
    else if( strcmp(first, "dev") == 0 ) {
        SHOW_LOG(3, fprintf(stdout, "sink: deveeeeeeeee\n"));
        receiver_config_dev_sink(&receiver, atoi(second));
    }
    else {
        EXIT_IF_TRUE(1, "wrong receiver spec\n");
    }

    node_media_config(&node, &streamer, &receiver);
    
    ///// Init gb-receiver
    memset(&gr, 0, sizeof(gr));
    gr.on_online_report_f = &on_online_report;
    gr.on_tx_report_f = &on_tx_report;
    gr.on_rx_report_f = &on_rx_report;
    gr.on_sq_report_f = &on_sq_report;
    gb_receiver_init(&gr, gb_cs);
    sleep(1);

    ////// periodically register
    n = pthread_create(&thread, NULL, auto_register, &node);
    EXIT_IF_TRUE(n != 0, "Error create a thread\n");

    ////// Main loop
    while (1) {
        if (fgets(option, sizeof(option), stdin) == NULL ) {
            SHOW_LOG(5, fprintf(stdout,"NULL cmd"));
        }
        switch(option[0]) {
            case 'j':
                node_invite(&node, guest);
                break;
            case 'l':
                node_repulse(&node, guest);
                break;
            case 'r':
                node_register(&node);
                break;
            case 't':
                node_start_session(&node);
                break;
            case 'd':
                node_pause(&node);
                break;
            case 'e':
                node_resume(&node);
                break;
            case '1':
                gb_server_join(&gr.gb_server, GB_MIP);
                break;
            case '2':
                gb_server_leave(&gr.gb_server, GB_MIP);
                break;
            default:
                SHOW_LOG(5,fprintf(stdout,"Unknow command\n"));
                break;
        }
    }
    return 0;
}
