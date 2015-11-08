#include "ansi-utils.h"
#include "gb-sender.h"
#include "proto-constants.h"

void gb_sender_create(gb_sender_t *gs, char *gb_cs2) {
#if 0
    int n;
    char gb_cs[30];
    memset(&gs, 0, sizeof(gs));
    n = sprintf(gb_cs, "udp:%s:%d", GB_MIP, GB_PORT);
    gb_cs[n] = '\0';

    gb_client_open(gs, gb_cs);
#endif

#if 1
    gb_client_open(gs, gb_cs2);
#endif
}
void gb_sender_report_online(gb_sender_t *gs, char *id, char *desc, int radio_port, int is_online) {
    gb_request_t req;
    req.msg_id = O_REPT;
    ansi_copy_str(req.o_rept.o_id, id);
    ansi_copy_str(req.o_rept.desc, desc);
    req.o_rept.radio_port = radio_port;
    req.o_rept.is_online = is_online;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_online:");
}

void gb_sender_report_tx(gb_sender_t *gs, char *id, int port, int is_tx){
    int n;
    gb_request_t req;
    req.msg_id = T_REPT;
    n = sprintf(req.t_rept.t_id, "%s", id);
    req.t_rept.t_id[n] = '\0';
    req.t_rept.is_tx = is_tx;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_tx:");
}

void gb_sender_report_rx(gb_sender_t *gs, char *id, int port, int is_rx){
    int n;
    gb_request_t req;
    req.msg_id = R_REPT;
    n = sprintf(req.r_rept.r_id, "%s", id);
    req.r_rept.r_id[n] = '\0';
    req.r_rept.is_rx = is_rx;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_rx:");
}

void gb_sender_report_sq(gb_sender_t *gs, char *id, int port, int is_sq){
    int n;
    gb_request_t req;

    req.msg_id = Q_REPT;
    n = sprintf(req.q_rept.q_id, "%s", id);
    req.q_rept.q_id[n] = '\0';
    //ansi_copy_str(req.q_rept.q_id, id);
    req.q_rept.is_sq = is_sq;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_sq:");
}
