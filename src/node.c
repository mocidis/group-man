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
    int n;
    gm_request_t req;
    
    //Set request value
    req.msg_id = GM_REG;
    memset(req.gm_reg.reg_id, 0, sizeof(req.gm_reg.reg_id));
    if (node->radio_port < 0) {
        strncpy(req.gm_reg.reg_id, node->id, strlen(node->id));
    }
    else {
        n = sprintf(req.gm_reg.reg_id, "%s%d", node->id, node->radio_port);
        req.gm_reg.reg_id[n] = '\0';
    }
    memset(req.gm_reg.gmc_cs, 0, sizeof(req.gm_reg.gmc_cs));
    strncpy(req.gm_reg.gmc_cs, node->gmc_cs, strlen(node->gmc_cs));
    memset(req.gm_reg.location, 0, sizeof(req.gm_reg.location));
    strncpy(req.gm_reg.location, node->location, strlen(node->location));
    memset(req.gm_reg.desc, 0, sizeof(req.gm_reg.desc));
    strncpy(req.gm_reg.desc, node->desc, strlen(node->desc));

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
                SHOW_LOG(4, fprintf(stdout, "Join %s\n", request->gmc_group.adv_ip));
                adv_server_join(&node->adv_server, request->gmc_group.adv_ip);
            }
            else if (request->gmc_group.join == 0) {
                SHOW_LOG(4, fprintf(stdout, "Leave %s\n", request->gmc_group.adv_ip));
                adv_server_leave(&node->adv_server, request->gmc_group.adv_ip);
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
    memset(node->id, 0, sizeof(node->id));
    strncpy(node->id, id, strlen(id));
    memset(node->gmc_cs, 0, sizeof(node->gmc_cs));
    strncpy(node->gmc_cs, gmc_cs, strlen(gmc_cs));
    memset(node->location, 0, sizeof(node->location));
    strncpy(node->location, location, strlen(location));
    memset(node->desc, 0, sizeof(node->desc));
    strncpy(node->desc, desc, strlen(desc));

    node->radio_port = radio_port;

    gm_client_open(&node->gm_client, gm_cs);

    memset(&node->gmc_server, 0, sizeof(node->gmc_server));
    memset(&node->adv_server, 0, sizeof(node->adv_server));

    node->gmc_server.on_request_f = &on_request_gmc_server;
    node->gmc_server.user_data = node;
    
    node->adv_server.on_request_f = node->on_adv_info_f;
    node->adv_server.user_data = node;

    gmc_server_init(&node->gmc_server, gmc_cs);
    adv_server_init(&node->adv_server, adv_cs);

    gmc_server_start(&node->gmc_server);
    adv_server_start(&node->adv_server);
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
    node->adv_server.is_online = 0;
}

void node_resume(node_t *node) {
    node->gmc_server.is_online = 1;
    node->adv_server.is_online = 1;
}
