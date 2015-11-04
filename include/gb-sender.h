#ifndef __GB_SENDER_H__
#define __GB_SENDER_H__

#include "gb-client.h"

typedef gb_client_t gb_sender_t;

void gb_sender_create(gb_sender_t *gs, char *gb_cs);
void gb_sender_report_online(gb_sender_t *gs, char *id, char *desc, int radio_port, int is_online);
void gb_sender_report_tx(gb_sender_t *gs, char *id, int port, int is_tx);
void gb_sender_report_rx(gb_sender_t *gs, char *id, int port, int is_rx);
void gb_sender_report_sq(gb_sender_t *gs, char *id, int port, int is_sq);

#endif

