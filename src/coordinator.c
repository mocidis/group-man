#include "ansi-utils.h"
#include "gm-server.h"
#include "gmc-client.h"
#include "adv-client.h"
#include "proto-constants.h"
#include "utlist.h"
#include "object-pool.h"
#include "gb-sender.h"

#include <time.h>

#include "pjlib.h"

#define ADV_MIP_LAST_OCTET_BEGIN 1

typedef struct entry_s entry_t;

struct entry_s {
    char id[20];
    char location[30];
    char desc[50];
    int radio_port;
    double frequency;
    double volume;
    
    time_t recv_time;

    gmc_client_t gmc_client;
    adv_client_t adv_client;

    entry_t *next;
    entry_t *prev;
};

typedef struct coordinator_s coordinator_t;

struct coordinator_s{
    opool_t opool;
    pj_pool_t *pool;
    int adv_mip_cnt;
    gm_server_t gm_server;
    gb_sender_t gb_sender;
    entry_t *registered_nodes; // List of registered nodes

    pj_thread_t *thread;   
};

coordinator_t coordinator;

static int id_cmp(entry_t *n1, entry_t *n2) {
    return strncmp(n1->id, n2->id, sizeof(n2->id));
}

int is_riu(entry_t *entry) { // Utility function
    return entry->radio_port >= 0;
}

static entry_t *find_entry_by_id(char *id) {
    entry_t *to_return;
	entry_t temp;
	strncpy(temp.id, id, strlen(id)+1);
    DL_SEARCH(coordinator.registered_nodes, to_return, &temp, id_cmp);
    return to_return;
}

static void gen_random_adv_cs(entry_t *entry, char *adv_cs) {
    int n = sprintf(adv_cs, "udp:239.0.0.%d:%d", coordinator.adv_mip_cnt++, ADV_PORT);
    adv_cs[n] = '\0';
}

void on_request(gm_server_t *gm_server, gm_request_t *request, char *caddr_str) {

    entry_t *temp, *temp2, *entry;
    char adv_cs[30], gm_cs[30], port[4];
    opool_item_t * item = NULL;
    time_t timer;

    gmc_request_t gmc_req;
    gmc_req.msg_id = GMC_GROUP;

    int n;

    SHOW_LOG(5, "Receive something\n");
    switch(request->msg_id) {
        case GM_REG:
            SHOW_LOG(5, "Receive request: From: %s Addr: %s\n", 
                    request->gm_reg.reg_id, request->gm_reg.gmc_cs);
            // Add entry in the request to coordinator.registered_nodes
            temp = find_entry_by_id(request->gm_reg.reg_id);

            time(&timer);

            if( temp != NULL ) {
                entry = temp;
                entry->recv_time = timer;
                //entry_update(temp, entry);
            }
            else {
                item = opool_get(&coordinator.opool);
                EXIT_IF_TRUE(item == NULL, "Cannot get from object pool\n");
                temp = (entry_t *)item->data;

                ansi_copy_str(temp->id, request->gm_reg.reg_id);
                ansi_copy_str(temp->location, request->gm_reg.location);
                ansi_copy_str(temp->desc, request->gm_reg.desc);
                temp->radio_port = request->gm_reg.radio_port;
                temp->frequency = request->gm_reg.frequency;
                temp->volume = request->gm_reg.volume;

                memset(&temp->gmc_client, 0, sizeof(gmc_client_t));

                extract_port(port, request->gm_reg.gmc_cs);
                n = sprintf(gm_cs ,"udp:%s:%s", caddr_str, port);
                gm_cs[n] = '\0';
                printf("gm_cs = %s\n", gm_cs);
                gmc_client_open(&temp->gmc_client, gm_cs);

                memset(&temp->adv_client, 0, sizeof(adv_client_t));
                gen_random_adv_cs(temp, adv_cs);
                adv_client_open(&temp->adv_client, adv_cs);

                temp->recv_time = timer;

                DL_APPEND(coordinator.registered_nodes, temp);

                gmc_req.gmc_group.join = 1;

                SHOW_LOG(3, "GM_REG: Node %s - gmc_cs:%s - adv_cs:%s \n",temp->id, temp->gmc_client.connect_str, temp->adv_client.connect_str);
            }

            break;
        case GM_GROUP:
            SHOW_LOG(3, "%s %s %s\n", request->gm_group.owner,\
                    request->gm_group.join?"invite":"repulse", request->gm_group.guest);
            //gmc_request_t gmc_req;
            //gmc_req.msg_id = GMC_GROUP;
            gmc_req.gmc_group.join = request->gm_group.join;
            ansi_copy_str(gmc_req.gmc_group.owner, request->gm_group.owner);

            //find owner's entry for adv_cs
            temp = find_entry_by_id(request->gm_group.owner);
            if (temp == NULL) {
                SHOW_LOG(2, "Error ID not found!\n");
                break;
            }

            extract_ip(temp->adv_client.connect_str, gmc_req.gmc_group.adv_ip);

            //find guest's entry
            DL_FOREACH(coordinator.registered_nodes, temp2) {
                if (strstr(temp2->id, request->gm_group.guest)) {
                    SHOW_LOG(3, "Tell %s(%s) join into ip %s\n", temp2->id, temp2->gmc_client.connect_str, gmc_req.gmc_group.adv_ip);
                    gmc_client_send(&temp2->gmc_client, &gmc_req);
                }
                /*
                   if (temp2 == NULL) {
                   SHOW_LOG(2,"Error ID not found!\n");
                   break;
                   }
                 */
            }
            break;
        case GM_INFO:
            SHOW_LOG(3, "Receive GM_INFO from %s: sdp_mip: %s sdp_port: %d\n", request->gm_info.gm_owner, request->gm_info.sdp_mip, request->gm_info.sdp_port);

            temp = find_entry_by_id(request->gm_info.gm_owner);
            if (temp == NULL) {
                SHOW_LOG(3, "Error Owner ID not found for %s!\n", request->gm_group.owner);
                break;
            }
            else {
                SHOW_LOG(3, "Onwer: %s\n", temp->id);
            }

            adv_request_t req;
            req.msg_id = ADV_INFO;
            ansi_copy_str(req.adv_info.adv_owner, request->gm_info.gm_owner);
            ansi_copy_str(req.adv_info.sdp_mip, request->gm_info.sdp_mip);
            req.adv_info.sdp_port = request->gm_info.sdp_port;

            SHOW_LOG(3, "Send ADV_INFO to %s\n", temp->adv_client.connect_str);
            adv_client_send(&temp->adv_client, &req);
            break;

        case GM_GET_INFO:
            SHOW_LOG(3, "Receive request: From: %s \n", request->gm_get_info.owner_id);
            temp = find_entry_by_id(request->gm_get_info.owner_id);
            if (temp == NULL) {
                SHOW_LOG(2, "Error ID not found!\n");
                break;
            }
            gmc_req.gmc_group.join = 1;
            //if (strstr(temp->id, "FTW")) {
            DL_FOREACH(coordinator.registered_nodes, temp2) {
                if (strstr(temp2->id, "RIUC")) {
                    ansi_copy_str(gmc_req.gmc_group.owner, temp2->id);
                    extract_ip(temp2->adv_client.connect_str, gmc_req.gmc_group.adv_ip);
                    SHOW_LOG(3, "Auto: Tell %s(%s) join into ip %s\n", temp->id, temp->gmc_client.connect_str, gmc_req.gmc_group.adv_ip);
                    gmc_client_send(&temp->gmc_client, &gmc_req);
                }
            }
            //}
            break;

        default:
                EXIT_IF_TRUE(1,"Unknown msg\n");
    }
}

int coordinator_proc(void *coord) {
    coordinator_t *coordinator = (coordinator_t *)coord;
    entry_t *temp, *entry;
    int is_online = 0;
    time_t timer;
    while(1) {
        if (coordinator->registered_nodes != NULL) {
            DL_FOREACH_SAFE(coordinator->registered_nodes, temp, entry) {
                time(&timer);

                is_online = (timer - temp->recv_time < 15)?1:0;
                SHOW_LOG(3, "%s is %s\n", temp->id, is_online?"online":"offline");
                
                gb_sender_report_online(&coordinator->gb_sender, temp->id, temp->desc, temp->radio_port, is_online);
            }
        }
        SHOW_LOG(5, "Coordinator_proc: Sending...\n");
        pj_thread_sleep(5*1000);
    }
    return 0;
}

void coordinator_auto_send(coordinator_t *coordinator) {
    pj_thread_create(coordinator->pool, "", coordinator_proc, coordinator, PJ_THREAD_DEFAULT_STACK_SIZE, 0, &coordinator->thread);
}

void usage(char *app) {
    printf("usage: %s <gm_server_cs>\n", app);
    exit(-1);
}

int main(int argc, char * argv[]) {
    if (argc < 2) usage(argv[0]);
    char *gm_server_cs;
    char gb_cs[30];
    int n;

    pj_caching_pool cp;
    pj_pool_t *pool;


    pj_init();
    pj_caching_pool_init(&cp, 0, 4096);
    pool = pj_pool_create(&cp.factory, "", 256, 256, NULL);

    SET_LOG_LEVEL(3);
    coordinator.pool = pool;
    coordinator.adv_mip_cnt = ADV_MIP_LAST_OCTET_BEGIN;

    SHOW_LOG(5, "Init object pool\n");
    opool_init(&coordinator.opool, 40, sizeof(entry_t), pool);

    // init gm_server
    gm_server_cs = argv[1];
    memset(&coordinator.gm_server, 0, sizeof(coordinator.gm_server));
    coordinator.gm_server.on_request_f = &on_request;

    SHOW_LOG(5, "Init gm server\n");
    gm_server_init(&coordinator.gm_server, gm_server_cs, pool);
    SHOW_LOG(5, "Start gm server\n");
    gm_server_start(&coordinator.gm_server);

    memset(&coordinator.gb_sender, 0, sizeof(coordinator.gb_sender));
    n = sprintf(gb_cs, "udp:%s:%d", GB_MIP, GB_PORT);
    gb_cs[n] = '\0';
    gb_sender_create(&coordinator.gb_sender, gb_cs);

    coordinator_auto_send(&coordinator);

    while (1) {
        pj_thread_sleep(1000);
    }
    return 0;
}
