#include "ansi-utils.h"
#include "gb-sender.h"
#include "proto-constants.h"

void gb_sender_create(gb_sender_t *gs, char *gb_cs) {
    gb_client_open(gs, gb_cs);
}
void gb_sender_report_online(gb_sender_t *gs, char *id, int is_online) {
    gb_request_t req;
    req.msg_id = O_REPT;
    ansi_copy_str(req.o_rept.o_id, id);
    req.o_rept.is_online = is_online;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_online:");
}

void gb_sender_report_tx(gb_sender_t *gs, char *id, int is_tx){
    gb_request_t req;
    req.msg_id = T_REPT;
    ansi_copy_str(req.t_rept.t_id, id);
    req.t_rept.is_tx = is_tx;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_tx:");
}

void gb_sender_report_rx(gb_sender_t *gs, char *id, int is_rx){
    gb_request_t req;
    req.msg_id = R_REPT;
    ansi_copy_str(req.r_rept.r_id, id);
    req.r_rept.is_rx = is_rx;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_rx:");
}

void gb_sender_report_sq(gb_sender_t *gs, char *id, int is_sq){
    gb_request_t req;
    req.msg_id = Q_REPT;
    ansi_copy_str(req.q_rept.q_id, id);
    req.q_rept.is_sq = is_sq;
    PERROR_IF_TRUE(gb_client_send(gs, &req) < 0, "ERROR::gb_sender_report_sq:");
}
