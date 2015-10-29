#ifndef __GB_RECEIVED_H
#define __GB_RECEIVED_H

#include "gb-server.h"

typedef struct {
    gb_server_t gb_server;
    void (* on_online_report_f)(char *id, int is_online);
    void (* on_tx_report_f)(char *id, int is_tx);
    void (* on_rx_report_f)(char *id, int is_rx);
    void (* on_sq_report_f)(char *id, int is_sq);
}gb_receiver_t;

void gb_receiver_init(gb_receiver_t *gr, char *gb_cs);
void gb_receiver_pause(gb_receiver_t *gr);
void gb_receiver_resume(gb_receiver_t *gr);
int gb_receiver_is_online(gb_receiver_t *gr);
#endif
