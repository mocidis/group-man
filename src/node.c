#include "ansi-utils.h"
#include "my-pjlib-utils.h"
#include "gm-client.h"
#include "gmc-server.h"
#include "adv-server.h"

#include "node.h"

void on_adv_info(adv_server_t *adv_server, adv_request_t *request, char *caddr_str) {
    node_t *node = adv_server->user_data;
    SHOW_LOG(3, "New session: %s(%s:%d)\n", request->adv_info.adv_owner, request->adv_info.sdp_mip, request->adv_info.sdp_port);
    int i, idx; 

    if(!node_has_media(node)) {
        SHOW_LOG(1, "Node does not have media endpoints configured\n");
        return;
    }

    idx = ht_get_item(&node->group_table, request->adv_info.adv_owner);

    if (idx >=0) {
        SHOW_LOG(4, "=========owner = %s, idx = %d\n", request->adv_info.adv_owner, idx);
        if( request->adv_info.sdp_port > 0 ) {
            receiver_stop(node->receiver, idx);

            //for (i = 0; i < node->receiver->nstreams; i++) {
                receiver_config_stream(node->receiver, request->adv_info.sdp_mip, request->adv_info.sdp_port, idx);
            //}

            receiver_start(node->receiver);
        }
        else {
            receiver_stop(node->receiver, idx);
        }
    }
    else {
        SHOW_LOG(1, "No record in group table for owner: %s (Idx = %d) ", request->adv_info.adv_owner, idx);
    }
}

void on_open_socket_adv_server(adv_server_t *adv_server) {
    static pj_thread_desc s_desc;
    static pj_thread_t *s_thread;
    ANSI_CHECK(__FILE__, pj_thread_register("adv_server", s_desc, &s_thread));
}

void on_request_gmc_server(gmc_server_t *gmc_server, gmc_request_t *request, char *caddr_str) {
    node_t *node = gmc_server->user_data;
    SHOW_LOG(5, "Receive something\n");
    int check_exit = 0, idx;
    
    idx = ht_get_size(&node->group_table);
    
    switch(request->msg_id) {
        case GMC_GROUP:
            SHOW_LOG(4, "Received request:\nFrom: %s\nAction: %d - Adv_ip: %s\n", 
                request->gmc_group.owner, request->gmc_group.join, request->gmc_group.adv_ip);
            if (request->gmc_group.join == 1) {
                SHOW_LOG(1, "idx = %d nstreams = %d\n", idx, node->receiver->nstreams);
                if (idx == node->receiver->nstreams) {
                    SHOW_LOG(1, "Caution: idx = nstreams: Number of streams reach maximum!\n");
                    ht_list_item(&node->group_table);
                }
                else {
                    SHOW_LOG(4, "%s join %s\n", node->id, request->gmc_group.adv_ip);
                    adv_server_join(node->adv_server, request->gmc_group.adv_ip);

                    check_exit = ht_get_item(&node->group_table, request->gmc_group.owner);
                    if (check_exit < 0) {
                        ht_add_item(&node->group_table, request->gmc_group.owner, &node->ht_idx[idx]);
                    }
                }
            }
            else if (request->gmc_group.join == 0) {
                ht_remove_item(&node->group_table, request->gmc_group.owner);
                if (node->on_leaving_server_f != NULL) {
                    node->on_leaving_server_f(request->gmc_group.owner, request->gmc_group.adv_ip);
                }
            }
            else {
                EXIT_IF_TRUE(1, "Unknown action\n");
            }
            break;
        default:
            EXIT_IF_TRUE(1, "Unknown request\n");
    }
}

void node_init(node_t *node, char *id, char *location, char *desc, int radio_port, char *gm_cs, char *gmc_cs, pj_pool_t *pool) {
    int i, n;

    SHOW_LOG(3, "++%s++%s++%s++%d++%s++%s\n", id, location, desc, radio_port, gm_cs, gmc_cs);

    if (radio_port < 0) {
        ansi_copy_str(node->id, id);
    }
    else {
        n = sprintf(node->id, "%s%d", id, radio_port);
        node->id[n] = '\0';
    }

    for (i = 0; i < MAX_STREAMS; i++) {
        node->ht_idx[i] = i;
    }

    ansi_copy_str(node->location, location);
    ansi_copy_str(node->desc, desc);
    ansi_copy_str(node->gmc_cs, gmc_cs);

    node->radio_port = radio_port;

    node->streamer = node->receiver = NULL;

    gm_client_open(&node->gm_client, gm_cs);

    memset(&node->gmc_server, 0, sizeof(node->gmc_server));

    node->gmc_server.on_request_f = &on_request_gmc_server;
    node->gmc_server.user_data = node;
    gmc_server_init(&node->gmc_server, gmc_cs, pool);
    gmc_server_start(&node->gmc_server);

    SHOW_LOG(3, "INIT HASH TABLE...STARTED\n");
    ht_init(&node->group_table, pool); 
    ht_create(&node->group_table, MAX_STREAMS);
    SHOW_LOG(3, "INIT HASH TABLE...DONE\n");
}

void node_media_config(node_t *node, endpoint_t *streamer, endpoint_t *receiver) {
    node->streamer = streamer;
    node->receiver = receiver;
}

int node_is_online(node_t *node) {
    return node->gmc_server.is_online && node->gmc_server.is_online;
}

int node_has_media(node_t *node) {
    return node->streamer != NULL && node->receiver != NULL;
}

void node_register(node_t *node) {
    if( !node_is_online(node) ) {
        SHOW_LOG(5, "Registration is failed: Node is not online\n");
        return;
    }
    gm_request_t req;
    
    //Set request value
    req.msg_id = GM_REG;

    ansi_copy_str(req.gm_reg.reg_id, node->id);
    ansi_copy_str(req.gm_reg.gmc_cs, node->gmc_cs);
    ansi_copy_str(req.gm_reg.location, node->location);
    ansi_copy_str(req.gm_reg.desc, node->desc);

    req.gm_reg.radio_port = node->radio_port;    

    //Send MG_REQ
    PERROR_IF_TRUE(gm_client_send(&node->gm_client, &req) < 0, "ERROR:: registered failed - ");
}
void node_invite(node_t *node, char *guest) {
    gm_request_t req;
    if( node_is_online(node) ) {
        req.msg_id = GM_GROUP;
        req.gm_group.join = 1;
        ansi_copy_str(req.gm_group.owner, node->id);
        ansi_copy_str(req.gm_group.guest, guest);
        PERROR_IF_TRUE(gm_client_send(&node->gm_client, &req) < 0, "ERROR::node_invite: ");
    }
}
void node_repulse(node_t *node, char *guest) {
    gm_request_t req;
    if( node_is_online(node) ) {
        req.msg_id = GM_GROUP;
        req.gm_group.join = 0;
        ansi_copy_str(req.gm_group.owner, node->id);
        ansi_copy_str(req.gm_group.guest, guest);
        PERROR_IF_TRUE(gm_client_send(&node->gm_client, &req) < 0, "ERROR::node_repulse: ");
    }
}

void node_start_session(node_t *node) {
    char mcast[16];
    int port;
    gm_request_t request;

    if(!node_has_media(node)) return;

    // Generate random mcast ip
    int n = sprintf(mcast, "237.0.%d.%d", pj_rand() % 254, pj_rand() % 254);
    mcast[n] = '\0';
    // Generate int random port
    port = 1000 + 2 * (pj_rand() % 300 + 12);
   
    // Broadcast session info
    request.msg_id = GM_INFO;
    ansi_copy_str(request.gm_info.gm_owner, node->id);
    ansi_copy_str(request.gm_info.sdp_mip, mcast);
    request.gm_info.sdp_port = port;
    gm_client_send(&node->gm_client, &request);

    // Start media stream for the session
    streamer_stop(node->streamer);
    streamer_config_stream(node->streamer, 0, mcast, port);
    streamer_start(node->streamer);
}

void node_stop_session(node_t *node) {
    gm_request_t request;

    // Broadcast session info
    request.msg_id = GM_INFO;
    ansi_copy_str(request.gm_info.gm_owner, node->id);
    request.gm_info.sdp_port = -1;
    gm_client_send(&node->gm_client, &request);

    // Stop media stream for the session
    streamer_stop(node->streamer);
}

void node_pause(node_t *node) {
    node->gmc_server.is_online = 0;
    node->adv_server->is_online = 0;
    node_stop_session(node);
}

void node_resume(node_t *node) {
    node->gmc_server.is_online = 1;
    node->adv_server->is_online = 1;
    node_start_session(node);
}

void node_add_adv_server(node_t *node, adv_server_t *adv_server) {
    node->adv_server = adv_server;
}

int node_in_group(node_t *node, char *owner_id) {
    return ht_get_item(&node->group_table, owner_id);
}

