#include "node.h"
#include <stdio.h>
#include "ansi-utils.h"
#include "gb-receiver.h"
#include "proto-constants.h"

void on_adv_info(adv_server_t *adv_server, adv_request_t *request) {
    SHOW_LOG(4, fprintf(stdout,"Received: ID = %s\nSDP addr %s:%d\n", request->adv_info.info_id, request->adv_info.sdp_mip, request->adv_info.sdp_port));
}

void on_online_report(char *id, int is_online) {
    SHOW_LOG(4, fprintf(stdout, "O_REPT:%s(online=%d)\n", id, is_online));
}
void on_tx_report(char *id, int is_tx) {
    SHOW_LOG(4, fprintf(stdout, "T_REPT:%s(tx=%d)\n", id, is_tx));
}
void on_rx_report(char *id, int is_rx) {
    SHOW_LOG(4, fprintf(stdout, "R_REPT:%s(rx=%d)\n", id, is_rx));
}
void on_sq_report(char *id, int is_sq) {
    SHOW_LOG(4, fprintf(stdout, "Q_REPT:%s(sq=%d)\n", id, is_sq));
}

void *auto_register(void *node_data) {
    node_t *node = (node_t *)node_data;
    while (1) {
        node_register(node);
        usleep(5*1000*1000);
    }
}

void usage(char *app) {
    printf("usage: %s <id> <location> <desc> <radio_port> <gm_cs> <gmc_cs> <guest>\n", app);
    exit(-1);
}

int main(int argc, char * argv[]) {
    node_t node;
    gb_receiver_t gr;

    if (argc < 8)
        usage(argv[0]);

    char *gm_cs;
    char *gmc_cs;
    char adv_cs[30];
    char option[10];
    char gb_cs[30];

    int adv_port = ADV_PORT;
    int gb_port = GB_PORT; 
    char *guest = argv[7];
    int n;

    pthread_t thread;
    
    SET_LOG_LEVEL(4);

    gm_cs = argv[5];
    gmc_cs = argv[6];
    n = sprintf(adv_cs, "udp:0.0.0.0:%d", adv_port);
    adv_cs[n] = '\0';
    n = sprintf(gb_cs, "udp:%s:%d",GB_MIP, gb_port);
    gb_cs[n] = '\0';

    SHOW_LOG(5, fprintf(stdout, "%s - %s - %s - %s - %s - %s - %s - %s\n",argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], adv_cs, gb_cs));

    memset(&node, 0, sizeof(node));
    node.on_adv_info_f = &on_adv_info;

    node_init(&node, argv[1], argv[2], argv[3], atoi(argv[4]), gm_cs, gmc_cs, adv_cs);

    memset(&gr, 0, sizeof(gr));
    gr.on_online_report_f = &on_online_report;
    gr.on_tx_report_f = &on_tx_report;
    gr.on_rx_report_f = &on_rx_report;
    gr.on_sq_report_f = &on_sq_report;
    gb_receiver_init(&gr, gb_cs);
    
    pthread_create(&thread, NULL, auto_register, &node) ;  


    ///////////////////////////////////////////////////////////
    // GM_INFO
    gm_request_t req_info;
    req_info.msg_id = GM_INFO;
    ansi_copy_str(req_info.gm_info.info_id, argv[1]);
    ansi_copy_str(req_info.gm_info.sdp_mip, "111.111.111.111");
    req_info.gm_info.sdp_port = 1111;
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
                PERROR_IF_TRUE(gm_client_send(&node.gm_client, &req_info) < 0, "ERROR::send failed - ");
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
