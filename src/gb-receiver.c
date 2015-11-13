#include "ansi-utils.h"
#include "proto-constants.h"
#include "gb-receiver.h"

static void on_request(gb_server_t *gb_server, gb_request_t *req) {
    //TODO
    gb_receiver_t *gr;
    SHOW_LOG(5, "gb_receiver: Received something\n");

    gr = gb_server->user_data;

    EXIT_IF_TRUE(gr == NULL, "gb_receiver: NULL\n");
    EXIT_IF_TRUE(gr->on_online_report_f == NULL, "Please specifi on_online_report_f callback\n");
    EXIT_IF_TRUE(gr->on_tx_report_f == NULL, "Please specifi on_tx_report_f callback\n");
    EXIT_IF_TRUE(gr->on_rx_report_f == NULL, "Please specifi on_rx_report_f callback\n");
    EXIT_IF_TRUE(gr->on_sq_report_f == NULL, "Please specifi on_sq_report_f callback\n");

    switch(req->msg_id) {
        case O_REPT:
            gr->on_online_report_f(req->o_rept.o_id, req->o_rept.desc, req->o_rept.radio_port, req->o_rept.is_online);   
            break;
        case T_REPT:
            gr->on_tx_report_f(req->t_rept.t_id, req->t_rept.is_tx);
            break;
        case R_REPT:
            gr->on_rx_report_f(req->r_rept.r_id, req->r_rept.is_rx);
            break;
        case Q_REPT:
            gr->on_sq_report_f(req->q_rept.q_id, req->q_rept.is_sq);
            break;
        default:
            EXIT_IF_TRUE(1, "gb_receiver: Unknown msg_id\n");
    }
}

void on_open_socket(gb_server_t *gb_server) {
    gb_server_join(gb_server, GB_MIP);
}

void gb_receiver_init(gb_receiver_t *gr, char *gb_cs) {
    memset(&gr->gb_server, 0, sizeof(gr->gb_server));    

    gr->gb_server.on_request_f = &on_request;
    gr->gb_server.on_open_socket_f = &on_open_socket;
    gr->gb_server.user_data = gr;

    gb_server_init(&gr->gb_server, gb_cs);
    gb_server_start(&gr->gb_server);
}

void gb_receiver_pause(gb_receiver_t *gr) {
    gr->gb_server.is_online = 0;
}

void gb_receiver_resume(gb_receiver_t *gr) {
    gr->gb_server.is_online = 1;
}

int gb_receiver_is_online(gb_receiver_t *gr) {
    return (gr->gb_server.is_online);
}
