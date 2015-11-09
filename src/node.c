#include "ansi-utils.h"
#include "gm-client.h"
#include "gmc-server.h"
#include "adv-server.h"

#include "node.h"

void node_register(node_t *node) {
    if( !node_is_online(node) ) {
        SHOW_LOG(5, fprintf(stdout, "Registration is failed: Node is not online\n"));
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
/*
void on_request_adv_server(adv_server_t *adv_server, adv_request_t *request) {
    //TODO
    SHOW_LOG(4, fprintf(stdout,"Received: ID = %s\nSDP addr %s:%d\n", request->adv_info.info_id, request->adv_info.sdp_mip, request->adv_info.sdp_port));
}
*/
void on_request_gmc_server(gmc_server_t *gmc_server, gmc_request_t *request) {
    node_t *node = gmc_server->user_data;
    SHOW_LOG(5, fprintf(stdout, "Receive something\n"));
    switch(request->msg_id) {
        case GMC_GROUP:
            SHOW_LOG(4, fprintf(stdout, "Received request:\nAction: %d - Adv_ip: %s\n", 
                    request->gmc_group.join, request->gmc_group.adv_ip));
            if (request->gmc_group.join == 1) {
                SHOW_LOG(4, fprintf(stdout, "%s join %s\n", node->id, request->gmc_group.adv_ip));
                adv_server_join(node->adv_server, request->gmc_group.adv_ip);
            }
            else if (request->gmc_group.join == 0) {
                SHOW_LOG(4, fprintf(stdout, "%s leave %s\n", node->id, request->gmc_group.adv_ip));
                adv_server_leave(node->adv_server, request->gmc_group.adv_ip);
            }
            else {
                EXIT_IF_TRUE(1, "Unknow action\n");
            }
            break;
        default:
            EXIT_IF_TRUE(1, "Unknow request\n");
    }
}

void node_init(node_t *node,char *id, char *location, char *desc, int radio_port, char *gm_cs, char *gmc_cs, char *adv_cs) {    
    int n;

    if (radio_port < 0) {
        ansi_copy_str(node->id, id);
    }
    else {
        n = sprintf(node->id, "%s%d", id, radio_port);
        node->id[n] = '\0';
    }
    ansi_copy_str(node->location, location);
    ansi_copy_str(node->desc, desc);
    ansi_copy_str(node->gmc_cs, gmc_cs);

    node->radio_port = radio_port;
    gm_client_open(&node->gm_client, gm_cs);

    memset(&node->gmc_server, 0, sizeof(node->gmc_server));
    //memset(&node->adv_server, 0, sizeof(node->adv_server));

    node->gmc_server.on_request_f = &on_request_gmc_server;
    node->gmc_server.user_data = node;
    
    gmc_server_init(&node->gmc_server, gmc_cs);
    gmc_server_start(&node->gmc_server);
}

int node_is_online(node_t *node) {
    return node->gmc_server.is_online && node->gmc_server.is_online;
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

void node_pause(node_t *node) {
    node->gmc_server.is_online = 0;
    node->adv_server->is_online = 0;
}

void node_resume(node_t *node) {
    node->gmc_server.is_online = 1;
    node->adv_server->is_online = 1;
}

void node_add_adv_server(node_t *node, adv_server_t *adv_server) {
    node->adv_server = adv_server;
}
