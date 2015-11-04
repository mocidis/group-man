#include "ansi-utils.h"
#include "gm-server.h"
#include "gmc-client.h"
#include "adv-client.h"
#include "proto-constants.h"
#include "utlist.h"
#include "object-pool.h"
#include "gb-sender.h"
#include <time.h>

#define ADV_MIP_LAST_OCTET_BEGIN 1

typedef struct entry_s entry_t;

struct entry_s {
    char id[10];
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
    int adv_mip_cnt;
    gm_server_t gm_server;
    gb_sender_t gb_sender;
    entry_t *registered_nodes; // List of registered nodes

    pthread_t thread;   
};

coordinator_t coordinator;

static int id_cmp(entry_t *n1, entry_t *n2) {
    return strncmp(n1->id, n2->id, sizeof(n2->id));
}

static int is_riu(entry_t *entry) { // Utility function
    return entry->radio_port >= 0;
}

static entry_t *find_entry_by_id(char *id) {
    entry_t *to_return;
    entry_t temp;
    strncpy(temp.id, id, strlen(id));
    DL_SEARCH(coordinator.registered_nodes, to_return, &temp, id_cmp);
    return to_return;
}

static void entry_update(entry_t *temp, entry_t *entry) {
    //entry_t *temp;
    SHOW_LOG(4, fprintf(stdout, "Node %s is updated\n", entry->id));
    DL_REPLACE_ELEM(coordinator.registered_nodes, temp, entry);
}

static void gen_random_adv_cs(entry_t *entry, char *adv_cs) {
    int n = sprintf(adv_cs, "udp:239.0.0.%d:%d", coordinator.adv_mip_cnt++, ADV_PORT);
    adv_cs[n] = '\0';
}

void on_request(gm_server_t *gm_server, gm_request_t *request) {

    entry_t *temp, *temp2, *entry;
    char adv_cs[30];
    opool_item_t * item = NULL;
    time_t timer;

    SHOW_LOG(5, fprintf(stdout, "Receive something\n"));
    switch(request->msg_id) {
        case GM_REG:
            SHOW_LOG(4, fprintf(stdout, "Receive request: From: %s Addr: %s\n", 
                        request->gm_reg.reg_id, request->gm_reg.gmc_cs));
            // Add entry in the request to coordinator.registered_nodes
            temp = find_entry_by_id(request->gm_reg.reg_id);

            gb_sender_report_online(&coordinator.gb_sender, request->gm_reg.reg_id, 1);

            time(&timer);

            if( temp != NULL ) {
                entry = temp;
                entry->recv_time = timer;
                entry_update(temp, entry);
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
                gmc_client_open(&temp->gmc_client, request->gm_reg.gmc_cs);

                memset(&temp->adv_client, 0, sizeof(adv_client_t));
                gen_random_adv_cs(temp, adv_cs);
                adv_client_open(&temp->adv_client, adv_cs);

                temp->recv_time = timer;

                DL_APPEND(coordinator.registered_nodes, temp);
                SHOW_LOG(4, fprintf(stdout,"Node %s - gmc_cs:%s - adv_cs:%s \n",temp->id, temp->gmc_client.connect_str, temp->adv_client.connect_str));
            }

            break;
        case GM_GROUP:
            SHOW_LOG(4, fprintf(stdout, "Receive request: ID=%d\n", request->msg_id));
            gmc_request_t gmc_req;

            gmc_req.msg_id = GMC_GROUP;
            gmc_req.gmc_group.join = request->gm_group.join;

            //find owner's entry for adv_cs
            temp = find_entry_by_id(request->gm_group.owner);
            if (temp == NULL) {
                SHOW_LOG(4, fprintf(stdout,"Error ID not found!\n"));
                break;
            }
            else {
                SHOW_LOG(4, fprintf(stdout, "Onwer: %s\n", temp->id));
            }

            extract_ip(temp->adv_client.connect_str, gmc_req.gmc_group.adv_ip);

            //find guest's entry
            temp2 = find_entry_by_id(request->gm_group.guest);
            if (temp2 == NULL) {
                SHOW_LOG(4, fprintf(stdout,"Error ID not found!\n"));
                break;
            }
            else {
                SHOW_LOG(4, fprintf(stdout, "Guest: %s\n", temp2->id));
            }

            SHOW_LOG(4, fprintf(stdout, "Tell %s join into ip %s\n", temp2->id, gmc_req.gmc_group.adv_ip));
            gmc_client_send(&temp2->gmc_client, &gmc_req);
            break;
        case GM_INFO:
            SHOW_LOG(4, fprintf(stdout, "Receive request: ID=%d\n", request->msg_id));

            temp = find_entry_by_id(request->gm_info.info_id);
            if (temp == NULL) {
                SHOW_LOG(4, fprintf(stdout,"Error ID not found!\n"));
                break;
            }

            adv_request_t req;
            req.msg_id = ADV_INFO;
            ansi_copy_str(req.adv_info.info_id, request->gm_info.info_id);
            ansi_copy_str(req.adv_info.sdp_mip, request->gm_info.sdp_mip);
            req.adv_info.sdp_port = request->gm_info.sdp_port;
       
            SHOW_LOG(4, fprintf(stdout, "Send ADV info to %s at %s\n", temp->id, temp->adv_client.connect_str));     
            adv_client_send(&temp->adv_client, &req);
            break;
        default:
            EXIT_IF_TRUE(1,"Unknow msg\n");
    }
}

void *coordinator_proc(void *coord) {
    coordinator_t *coordinator = (coordinator_t *)coord;
    entry_t *temp, *entry;
    int is_online = 0;
    time_t timer;
    while(1) {
        if (coordinator->registered_nodes != NULL) {
            DL_FOREACH_SAFE(coordinator->registered_nodes, temp, entry) {
                time(&timer);
                
                is_online = (timer - temp->recv_time < 15)?1:0;

                gb_sender_report_online(&coordinator->gb_sender, temp->id, is_online);
            }
        }
        SHOW_LOG(5, fprintf(stdout, "Coordinator_proc: Sending...\n"));
        usleep(5*1000*1000);
    }
}

void coordinator_auto_send(coordinator_t *coordinator) {
    pthread_create(&coordinator->thread, NULL, coordinator_proc, coordinator);
}

void usage(char *app) {
    printf("usage: %s <gm_server_cs>\n", app);
    exit(-1);
}

int main(int argc, char * argv[]) {
    if (argc < 2)
        usage(argv[0]);
    char *gm_server_cs;
    char gb_cs[30];
    int n;

    SET_LOG_LEVEL(4);
    coordinator.adv_mip_cnt = ADV_MIP_LAST_OCTET_BEGIN;

    SHOW_LOG(5, fprintf(stdout, "Init object pool\n"));
    opool_init(&coordinator.opool, 40, sizeof(entry_t));
   
    // init gm_server
    gm_server_cs = argv[1];
    memset(&coordinator.gm_server, 0, sizeof(coordinator.gm_server));
    coordinator.gm_server.on_request_f = &on_request;
   
    SHOW_LOG(5, fprintf(stdout, "Init gm server\n"));
    gm_server_init(&coordinator.gm_server, gm_server_cs);
    SHOW_LOG(5, fprintf(stdout, "Start gm server\n"));
    gm_server_start(&coordinator.gm_server);

    memset(&coordinator.gb_sender, 0, sizeof(coordinator.gb_sender));
    n = sprintf(gb_cs, "udp:%s:%d", GB_MIP, GB_PORT);
    gb_cs[n] = '\0';
    gb_sender_create(&coordinator.gb_sender, gb_cs);
 
    coordinator_auto_send(&coordinator);
   
    while (1) {
        sleep(1);
    }
    return 0;
}
