#include "network/gestionary-com.h"
#include "network/p2p-msg.h"
#include "utils/genericlist.h"
#include "utils/message.h"
#include <bits/pthreadtypes.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/logger.h>

#define CERT_PATH                                                                                  \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define KEY_PATH                                                                                   \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

#define DEFAULT_PORT_P2P 12000
#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

typedef struct s_client {
    char user_id[SIZE_NAME];
    Gestionary_com *gest;
    char request[SIZE_NAME]; /* user that request this user */
    bool available;          /* available to connect */
    char public_ip[SIZE_IP_CHAR];
    char private_ip[SIZE_IP_CHAR];
    int private_port;
    int public_port;
} Client;

/* informations about 2 peers trying to connect */
typedef struct s_in_connction {
    Client *tab[2];
} InConnection;

int port_server;
char ip_server[SIZE_IP_CHAR];
GenList *listConnected;
pthread_mutex_t mutex_list;
TLS_infos *tls;

/* get args from main launch */
void getArgs(int argc, char *argv[]);
/* create struct InConnections */
InConnection *createInConnection(Client *client0, Client *client1);
/* delete struct InConnections */
void deleteInConnection(InConnection *inco);
/* Create struct client */
Client *createClient(char *user_id, Gestionary_com *gest);
/* delete struct client */
void deleteClient(Client *client);
/* add informations about peer 0 or 1*/
int inConnectionAddInformations(InConnection *inco, P2P_msg *infos, unsigned peer);
/* start protocol to establish communication between two peers */
void connectPeers(InConnection *infos);
/* listen for new connections and add them to the buffer of connected */
void *threadListenNewConnections(void *arg);
/* Remove client from the list */
void removeClientFromList(Client *client);
/* listen and respond to peers connectded */
void *threadListenRequests(void *arg);
/* send list of connected and available clients */
void manageRequestPeerConnected(Client *client);
/* manage request connection from client */
void manageRequestConnection(Client *client, P2P_msg *msg);
/* manage accept/reject connection from client */
void manageAcceptRejectConnection(Client *client, P2P_msg *msg);
/* manage request of peer trying to connect */
void manageRequestPeerRequest(Client *client);
/* manage unexpected request */
void manageUnexpectedRequest(Client *client);
/* find client */
Client *findClient(char *user_id);

/* debug */
void tryServer(char *act);
void okServer();

int main(int argc, char *argv[]) {
    init_logger(NULL);
    getArgs(argc, argv);
    unsigned long nbt1, nbt2;

    /* init var */
    listConnected = createGenList(8);
    pthread_mutex_init(&mutex_list, NULL);
    tls = initTLSInfos(ip_server, port_server, MAIN_SERVER, CERT_PATH, KEY_PATH);
    if (!tls) {
        warnl("main.c", "main", "fail init TLS info");
        return 1;
    }

    /* thread listen connections */
    if (pthread_create(&nbt1, NULL, threadListenNewConnections, NULL) != 0) {
        exitl("main.c", "main", 1, "fail pthread_crete connections");
    }

    /* thread listen request */
    if (pthread_create(&nbt2, NULL, threadListenRequests, NULL) != 0) {
        exitl("main.c", "main", 1, "fail pthread_crete requests");
    }

    printf("[SERVER] OPEN\n");

    pthread_join(nbt1, NULL);
    pthread_join(nbt2, NULL);

    close_logger();
    return 0;
}

void displayUsage(char *bin) {
    printf("usage : %s <ip_server> <port_server>\n", bin);
}

void getArgs(int argc, char *argv[]) {
    if (argc != 3) {
        displayUsage(argv[0]);
        exit(1);
    }
    strncpy(ip_server, argv[1], SIZE_IP_CHAR);
    port_server = atoi(argv[2]);
    if (port_server < 1024) {
        displayUsage(argv[0]);
        exit(1);
    }
}

/* create struct InConnections */
InConnection *createInConnection(Client *client1, Client *client2) {
    InConnection *inco = malloc(sizeof(InConnection));
    memset(inco, 0, sizeof(InConnection));
    inco->tab[0] = client1;
    inco->tab[1] = client2;
    return inco;
}

void deleteInConnection(InConnection *inco) {
    memset(inco->tab[0]->request, 0, SIZE_NAME);
    memset(inco->tab[1]->request, 0, SIZE_NAME);
    inco->tab[0]->available = true;
    inco->tab[1]->available = true;
    free(inco);
}

/* Create struct client */
Client *createClient(char *user_id, Gestionary_com *gest) {
    Client *client = malloc(sizeof(Client));
    memset(client, 0, sizeof(Client));
    strncpy(client->user_id, user_id, SIZE_NAME);
    client->available = true;
    client->gest = gest;
    client->private_port = -1;
    client->public_port = -1;
    return client;
}

/* delete struct client */
void deleteClient(Client *client) {
    deleteGestionaryCom(&client->gest);
    free(client);
}

/* add informations about peer 1 or 2*/
int inConnectionAddInformations(InConnection *inco, P2P_msg *infos, unsigned peer) {
    if (peer < 0 || peer > 1) {
        warnl("main.c", "inConnectionAddInformations", "invalid peer num");
        return -1;
    }
    if (infos->type != RESPONSE_INFO_COM) {
        warnl("main.c", "inConnectionAddInformations", "invalid P2P_msg type");
        return -1;
    }
    if (infos->public_port != -1)
        inco->tab[peer]->public_port = infos->public_port;
    if (infos->private_port != -1)
        inco->tab[peer]->private_port = infos->private_port;
    if (infos->private_ip[0] != 0)
        strncpy(inco->tab[peer]->private_ip, infos->private_ip, SIZE_IP_CHAR);
    return 0;
}

/* start protocol to establish communication between two peers */
void connectPeers(InConnection *infos) {
    /* set unavailable */
    infos->tab[0]->available = false;
    infos->tab[1]->available = false;

    /* get informations */
    P2P_msg *peerInfo;
    while (gestionaryReceiveP2P(infos->tab[0]->gest, &peerInfo) != 1)
        ;
    if (!peerInfo) {
    }

    inConnectionAddInformations(infos, peerInfo, 0);

    deleteP2PMsg(peerInfo);

    while (gestionaryReceiveP2P(infos->tab[1]->gest, &peerInfo) != 1)
        ;

    inConnectionAddInformations(infos, peerInfo, 1);

    deleteP2PMsg(peerInfo);

    /* try local network */
    P2P_msg *msg0 = createP2PMsg(TRY_SERVER_MODE);
    P2P_msg *msg1 = createP2PMsg(TRY_CLIENT_MODE);
    P2P_msg *rep0, *rep1;

    if (infos->tab[0]->private_port != -1) {

        setP2PTryInfo(msg0, infos->tab[0]->private_ip, infos->tab[0]->private_port);
        setP2PTryInfo(msg1, infos->tab[0]->private_ip, infos->tab[0]->private_port);
    } else {

        setP2PTryInfo(msg0, infos->tab[0]->private_ip, DEFAULT_PORT_P2P);
        setP2PTryInfo(msg1, infos->tab[0]->private_ip, DEFAULT_PORT_P2P);
    }

    gestionarySendP2P(infos->tab[0]->gest, msg0);
    sleep(1);
    gestionarySendP2P(infos->tab[1]->gest, msg1);

    deleteP2PMsg(msg0);
    deleteP2PMsg(msg1);

    while (gestionaryReceiveP2P(infos->tab[0]->gest, &rep0) != 1) {
        if (!gestionaryIsComOpen(infos->tab[0]->gest)) {
            break;
        }
    }

    while (gestionaryReceiveP2P(infos->tab[1]->gest, &rep1) != 1) {
        if (!gestionaryIsComOpen(infos->tab[1]->gest)) {
            break;
        }
    }

    if (!rep0 || !rep1) {

        if (!rep0) {
            removeClientFromList(infos->tab[0]);
            infos->tab[0] = NULL;
        } else {
            infos->tab[0]->available = true;
            deleteP2PMsg(rep0);
        }
        if (!rep1) {
            removeClientFromList(infos->tab[1]);
            infos->tab[1] = NULL;
        } else {
            infos->tab[1]->available = true;
            deleteP2PMsg(rep1);
        }
    }

    if (rep0->type != CON_SUCCESS || rep1->type != CON_SUCCESS) {
        deleteP2PMsg(rep0);
        deleteP2PMsg(rep1);
        msg0 = createP2PMsg(END);
        if (infos->tab[0])
            gestionarySendP2P(infos->tab[0]->gest, msg0);
        if (infos->tab[1])
            gestionarySendP2P(infos->tab[1]->gest, msg1);
        deleteP2PMsg(msg0);
        infos->tab[0]->available = true;
        infos->tab[1]->available = true;
    }
}

/* listen for new connections and add them to the buffer of connected */
void *threadListenNewConnections(void *arg) {
    while (1) {
        TLS_infos *client_tls = acceptComTLS(tls);
        if (!client_tls) {
            warnl("main.c", "threadListenNewConnections", "fail accept client");
            continue;
        }
        Gestionary_com *gest = createGestionaryCom(client_tls);
        if (!gest) {
            warnl("main.c", "threadListenNewConnections", "fail create gestionary");
            continue;
        }
        char *user_id;
        while (gestionaryReceiveTxt(gest, &user_id) != 1)
            ;
        fflush(stdout);
        Client *client = createClient(user_id, gest);
        genListAdd(listConnected, (void *)client);
        printf("[SERVER] New Connection : %s\n", client->user_id);
    }
    return NULL;
}

/* listen and respond to peers connectded */
void *threadListenRequests(void *arg) {
    unsigned cur = 0;
    int ret;
    P2P_msg *msg;
    Client *client;
    while (1) {
        // sleep(1);
        pthread_mutex_lock(&mutex_list);
        if (cur >= genListSize(listConnected)) {
            cur = -1;
        } else {
            client = (Client *)genListGet(listConnected, cur);
            if (client->available) {
                ret = gestionaryReceiveP2P(client->gest, &msg);
                if (ret == 1) {
                    printf("[SERVER] Request from %s : %d\n", client->user_id, msg->type);
                    switch (msg->type) {
                    case ACCEPT_CONNECTION:
                    case REJECT_CONNECTION:

                        manageAcceptRejectConnection(client, msg);
                        okServer();
                        break;
                    case REQUEST_CONNECTION:

                        manageRequestConnection(client, msg);
                        okServer();
                        break;
                    case REQUEST_LIST_ONLINE:

                        manageRequestPeerConnected(client);
                        okServer();
                        break;
                    case REQUEST_PEER_REQUEST:

                        manageRequestPeerRequest(client);
                        okServer();
                        break;
                    default:
                        manageUnexpectedRequest(client);
                    }
                } else if (ret < 0 || !gestionaryIsComOpen(client->gest)) {
                    removeClientFromList(client);
                } else {
                    /* TODO case end p2p connection, return mode available */
                }
            }
        }
        cur++;
        pthread_mutex_unlock(&mutex_list);
    }
}
/* send list of connected and available clients */
void manageRequestPeerConnected(Client *client) {
    P2P_msg *msg = createP2PMsg(RESPONSE_LIST_ONLINE);
    GenList *l = createGenList(genListSize(listConnected));
    Client *c;
    char *id;
    for (unsigned i = 0; i < genListSize(listConnected); i++) {
        c = genListGet(listConnected, i);
        id = malloc(SIZE_NAME);
        if (c != client && c->available) {
            strncpy(id, c->user_id, SIZE_NAME);
            genListAdd(l, id);
        }
    }
    setP2PListUserOnline(msg, l);
    gestionarySendP2P(client->gest, msg);
}
/* Remove client from the list */
void removeClientFromList(Client *client) {
    pthread_mutex_lock(&mutex_list);
    int i = 0;
    Client *cur_client;
    while (i < genListSize(listConnected) && (cur_client = genListGet(listConnected, i)) != client)
        i++;
    if (i >= genListSize(listConnected)) {
        pthread_mutex_unlock(&mutex_list);
        return;
    }
    genListRemove(listConnected, i);
    deleteClient(cur_client);
    pthread_mutex_unlock(&mutex_list);
}
/* manage request connection from client */
void manageRequestConnection(Client *client, P2P_msg *msg) {
    Client *request = findClient(msg->id_peer);
    P2P_msg *accept_msg = createP2PMsg(ACCEPT_CONNECTION);
    P2P_msg *reject_msg = createP2PMsg(REJECT_CONNECTION);

    if (!request || !request->available ||
        (request->request[0] != 0 && strncmp(request->request, client->user_id, SIZE_NAME) != 0)) {
        gestionarySendP2P(client->gest, reject_msg);
        return;
    }
    if (request->request[0] == 0) {
        strncpy(request->request, client->user_id, SIZE_NAME);
        return;
    }
    gestionarySendP2P(client->gest, accept_msg);
    gestionarySendP2P(request->gest, accept_msg);
    InConnection *inco = createInConnection(client, request);
    connectPeers(inco);
}

/* manage accept/reject connection from client */
void manageAcceptRejectConnection(Client *client, P2P_msg *msg) {
    P2P_msg *accept_msg = createP2PMsg(ACCEPT_CONNECTION);
    P2P_msg *reject_msg = createP2PMsg(REJECT_CONNECTION);
    if (client->request[0] == 0) {

        if (msg->type == ACCEPT_CONNECTION) {
            gestionarySendP2P(client->gest, reject_msg);
        }
        return;
    }

    Client *peer = findClient(client->request);
    okServer();
    if (!peer) {

        memset(client->request, 0, SIZE_NAME);
        if (msg->type == ACCEPT_CONNECTION) {
            gestionarySendP2P(client->gest, reject_msg);
        }
        return;
    }
    if (msg->type == REJECT_CONNECTION) {

        gestionarySendP2P(peer->gest, reject_msg);
        return;
    }

    gestionarySendP2P(client->gest, accept_msg);
    gestionarySendP2P(peer->gest, accept_msg);

    InConnection *inco = createInConnection(client, peer);
    connectPeers(inco);
    okServer();
}
/* manage request of peer trying to connect */
void manageRequestPeerRequest(Client *client) {
    P2P_msg *msg = createP2PMsg(RESPONSE_PEER_REQUEST);
    setP2PIdPeer(msg, client->request);
    gestionarySendP2P(client->gest, msg);
}

void manageUnexpectedRequest(Client *client) {
    warnl("main.c", "manageUnexpectedRequest", "invalide type");
}

/* find client */
Client *findClient(char *user_id) {
    Client *c;
    for (unsigned i = 0; i < genListSize(listConnected); i++) {
        c = genListGet(listConnected, i);
        if (c->available) {
            if (strncmp(c->user_id, user_id, SIZE_NAME) == 0) {
                return c;
            }
        }
    }
    return NULL;
}

void tryServer(char *act) {
    printf("\n %s[SERVER] > %s %s\n", BLUE, act, RESET);
    fflush(stdout);
}

void okServer() {
    printf("\t\t %s <+>---------- OK %s\n", GREEN, RESET);
    fflush(stdout);
}

// int main(int argc, char *argv[]) {
//     int ret;
//     Msg *msg;
//     TLS_infos *tls;
//     Gestionary_com *gest;
//     printf("[SERVER] Starting...\n \t <+>--- OK\n");

//     init_logger(NULL);
//     printf("[SERVER] get args...\n");
//     if (argc != 3) {
//         printl("usage : %s <ip_server> <port_server>\n", argv[0]);
//         return 1;
//     }
//     port = atoi(argv[2]);
//     strncpy(ip, argv[1], SIZE_IP_CHAR);
//     printf(" > IP : %s \n > Port : %d\n", ip, port);
//     printf(" \t <+>--- OK\n");

//     printf("[SERVER] initTLSInfos...\n");
//     tls = initTLSInfos(ip, port, SERVER, CERT_PATH, KEY_PATH);
//     printf(" \t <+>--- OK\n");

//     printf("[SERVER] openComTLS...\n");
//     if (openComTLS(tls) != 0) {
//         warnl("main", "main", "Echec connection");
//         return 1;
//     }
//     printf(" \t <+>--- OK\n");

//     printf("[SERVER] createGestionaryCom...\n");
//     gest = createGestionaryCom(tls);
//     printf(" \t <+>--- OK\n");

//     printf("[SERVER] Receive packet...\n");

//     /* receive packet */
//     while ((ret = gestionaryReceiveMsg(gest, &msg)) < 1) {
//         if (ret == -1 && gestionaryIsComOpen(gest) == false) {
//             warnl("main.c", "main", "error com closed");
//             return 1;
//         }
//     }
//     printf("[SERVER] message from <%s> : <%s>\n", msg->sender, msg->buffer);

//     /* send packet */
//     printf("[SERVER] Send packet...\n");
//     msg = malloc(sizeof(Msg));
//     memset(msg, 0, sizeof(Msg));
//     strncpy(msg->buffer, "hello from server !", SIZE_MSG_DATA);
//     strncpy(msg->sender, "SERVER", SIZE_NAME);
//     gestionarySendMsg(gest, msg);
//     printf(" \t <+>--- OK\n");

//     /* delete */
//     printf("[SERVER] deleteGestionaryCom...\n");
//     deleteGestionaryCom(&gest);
//     printf(" \t <+>--- OK\n");
//     close_logger();
//     printf("[SERVER] END\n");
//     return 0;
// }
