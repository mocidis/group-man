#ifndef __NODE_H__
#define __NODE_H__
#include "gm-client.h"
#include "gmc-server.h"
#include "adv-server.h"
#include "proto-constants.h"

#include "endpoint.h"
#include "hash-table.h"

#define MAX_STREAMS 20

typedef struct node_s {
    gm_client_t gm_client;
    gmc_server_t gmc_server;
    adv_server_t *adv_server;    
    
    char id[20];
    char gmc_cs[30];
    char location[30];
    char desc[50];

    int radio_port;
    unsigned streamer_dev_idx;
    unsigned receiver_dev_idx;

    endpoint_t *streamer;
    endpoint_t *receiver;

    hash_table_t group_table;
    int ht_idx[MAX_STREAMS];

    // Node's events
    void (*on_adv_info_f)(adv_server_t *adv_server, adv_request_t *request, char *caddr_str);
    void (*on_leaving_server_f)(char *owner_id, char *adv_ip);
} node_t ;

void node_init(node_t *node,
               char *id, 
               char *location, 
               char *desc, 
               int radio_port, 
               char *gm_cs, 
               char *gmc_cs, 
               pj_pool_t *pool);

void node_media_config(node_t *node, endpoint_t *streamer, endpoint_t *receiver);

// Node's control actions
void node_pause(node_t *node);
void node_resume(node_t *node);

// Node's status query actions
int node_is_online(node_t *node);
int node_has_media(node_t *node);

// Node's protocol actions
void node_register(node_t *node);
void node_invite(node_t *node, char *guest);
void node_repulse(node_t *node, char *guest);
void node_start_session(node_t *node);
void node_stop_session(node_t *node);

void *node_in_group(node_t *node, char *owner_id);

void node_add_adv_server(node_t *node, adv_server_t *adv_server);

// Callback function
void on_adv_info(adv_server_t *adv_server, adv_request_t *request, char *caddr_str);
void on_open_socket_adv_server(adv_server_t *adv_server);
#endif
