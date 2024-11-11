#include <server/request-handler.h>
#include <stdbool.h>
#include <unistd.h>
#include <utils/logger.h>
#include <types/list.h>
#include <server/database-manager.h>
#include <signal.h>

#define FILE_NAME  "request-handler"

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"



typedef struct s_tuple_client {
    Client *c1;
    Client *c2;
} Tuple_Client;

void tryServer(char *arg) {
    printf("\n %s[SERVER] > %s %s\n", BLUE, arg, RESET);
    fflush(stdout);
}

void okServer(char *arg) {
    printf("\t\t %s <+>---------- %s OK %s\n", GREEN, arg, RESET);
    fflush(stdout);
}

void koServer(char *arg) {
    printf("\t\t %s <+>---------- %s KO %s\n", RED, arg, RESET);
    fflush(stdout);
}

void *createP2Pconnection(void *arg) {
    Tuple_Client *info = arg;
    P2P_msg *msg = initP2PMsg(P2P_GET_INFOS, "serveur");
    Packet *p = initPacketP2PMsg(msg);
    Packet *receive1, *receive2;
    TLS_error error1, error2;
    TLS_infos *i1 = info->c1->info_user;
    TLS_infos *i2 = info->c2->info_user;

    tlsSend(i1, p);
    tlsSend(i2, p);

    printl(" demande des information envoyer\n");

    error1 = tlsReceiveBlocking(i1, &receive1);
    error2 = tlsReceiveBlocking(i2, &receive2);

    printl(" information recu\n");

    msg = initP2PMsg(P2P_TRY_SERVER_MODE, "serveur");

    int try_port = p2pMsgGetPrivatePort(&receive2->p2p);
    char *try_ip = p2pMsgGetPrivateIp(&receive2->p2p);
    p2pMsgSetTryInfo(msg, try_ip, try_port);
    p = initPacketP2PMsg(msg);
    tlsSend(i1, p);

    msg = initP2PMsg(P2P_TRY_CLIENT_MODE, "serveur");

    try_port = p2pMsgGetPrivatePort(&receive1->p2p);
    try_ip = p2pMsgGetPrivateIp(&receive1->p2p);
    p2pMsgSetTryInfo(msg, try_ip, try_port);
    p = initPacketP2PMsg(msg);
    tlsSend(i2, p);

    printl("connection etablie\n");

    info->c1->etat = IN_CONNECTION;
    info->c2->etat = IN_CONNECTION;
    listAdd(thread, pthread_self());
    pthread_kill(nb_main, SIGUSR1);
    return NULL;
}

void acceptHandler(Packet *p, Client *c) {
    int target_nb;
    pthread_t temp;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);
    printf("demande accepter\n");

    if (genListContainsString(c->request_by, target)) {
        printf("demande en cours\n");
        Tuple_Client *info = malloc(sizeof(Tuple_Client));
        info->c1 = clientListGetId(user, getId(conn, target));
        info->c2 = c;
        c->etat = TRY_CONNECTION;
        info->c1->etat = TRY_CONNECTION;
        pthread_create(&temp, NULL, createP2Pconnection, info);
    }
}

void rejecttHandler(Packet *p, Client *c) {
    int user_nb;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);

    if (genListContainsString(c->request_by, target)) {
        Packet *send;
        P2P_msg *msg;
        msg = initP2PMsg(P2P_REJECT, "serveur");
        p2pMsgSetPeerId(msg, sender);
        send = initPacketP2PMsg(msg);
        tlsSend(clientListGet(user, user_nb)->info_user, send);
        deinitPacket(&send);
        deinitP2PMsg(&msg);
    }
}

void requestP2PtHandler(Packet *p, Client *c) {
    int user_nb;
    Packet *send;
    P2P_msg *msg;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);
    printl(" > request from <%s> to <%s>", sender, target);

    Client *t = clientListGetId(user, getId(conn, target));

    if (t == NULL) {
        printl(" > target disconnected");
        msg = initP2PMsg(P2P_REJECT, "serveur");
        p2pMsgSetError(msg, P2P_ERR_USER_DISCONNECTED);
        send = initPacketP2PMsg(msg);
        tlsSend(c->info_user, send);
        deinitPacket(&send);
        deinitP2PMsg(&msg);
    } else {
        if (t->etat != AVAILABLE) {
            printf("t = %d\n", t->etat);
            printl(" > target unavailable");
            msg = initP2PMsg(P2P_REJECT, "serveur");
            p2pMsgSetError(msg, P2P_ERR_UNAVAILABLE_USER);
            send = initPacketP2PMsg(msg);
            tlsSend(c->info_user, send);
            deinitPacket(&send);
            deinitP2PMsg(&msg);
        } else {
            genListAdd(t->request_by, sender);
            printl(" > send request to target");
            msg = initP2PMsg(P2P_REQUEST_IN, "serveur");
            p2pMsgSetPeerId(msg, sender);
            send = initPacketP2PMsg(msg);

            tlsSend(t->info_user, send);
            deinitPacket(&send);
            deinitP2PMsg(&msg);
        }
    }
}

void getAvailableHandler(Packet *p, Client *c) {
    GenList *res = initGenList(clientListSize(user));
    Client *temp;
    for (unsigned i = 0; i < clientListSize(user); i++) {
        temp = clientListGet(user, i);
        if (temp != c)
            genListAdd(res, temp->username);
    }
    P2P_msg *msg = initP2PMsg(P2P_AVAILABLE, "server");
    p2pMsgSetListUserOnline(msg, res);

    Packet *send = initPacketP2PMsg(msg);

    tlsSend(c->info_user, send);

    deinitPacket(&send);
    deinitP2PMsg(&msg);
}

void *startConnection(void *arg) {
    TLS_infos *temp = arg;
    Client *c;
    Packet *receive;
    Packet *send;
    P2P_msg *msg_send;
    TLS_error error;
    char *sender;
    char *password_hash;
    bool connection_etablish = false;
    tryServer("startConnection tlsReceiveBlocking");
    while (!connection_etablish) {
        error = TLS_RETRY;
        while (error == TLS_RETRY) {
            error = tlsReceiveBlocking(temp, &receive);
            printf(" > error : %d\n", error);
        }
        tryServer("startConnection check error");
        if (error == TLS_SUCCESS) {
            if (receive->type == PACKET_P2P_MSG) {
                P2P_msg msg = receive->p2p;
                if (msg.type == P2P_CONNECTION_SERVER) {
                    sender = p2pMsgGetSenderId(&(receive->p2p));
                    password_hash = p2pMsgGetPasswordHash(&(receive->p2p));
                    if (login(conn, sender, password_hash)) {
                        c = malloc(sizeof(Client));
                        c->info_user = temp;
                        strncpy(c->username, sender, SIZE_NAME);
                        c->etat = AVAILABLE;
                        c->request_by = initGenList(1);
                        c->id = getId(conn, sender);
                        clientListAdd(user, c);
                        msg_send = initP2PMsg(P2P_CONNECTION_OK, "server");
                        p2pMsgSetError(msg_send, P2P_ERR_SUCCESS);
                        printf("user connected\n");
                        okServer("startConnection");
                        connection_etablish = true;
                    } else {
                        msg_send = initP2PMsg(P2P_CONNECTION_KO, "serveur");
                        p2pMsgSetError(msg_send, P2P_ERR_CONNECTION_FAILED);
                        koServer("startConnection");
                    }
                    free(sender);
                    free(password_hash);
                    send = initPacketP2PMsg(msg_send);
                    tlsSend(temp, send);
                }
            }
        } else {
            connection_etablish = true;
            deinitTLSInfos(&temp);
        }
    }
    tryServer("startConnection add thread to list");

    listAdd(thread, pthread_self());
    pthread_kill(nb_main, SIGUSR1);
    okServer("startConnection");

    return NULL;
}

void *accepteUser(void *arg) {
    char *FUN_NAME = "accepteUser";
    TLS_infos *temp, *tsl = arg;
    pthread_t num_t;
    while (!end) {
        tryServer("accepteUser tlsAcceptCom");
        temp = tlsAcceptCom(tsl);
        if (!temp) {
            warnl(FILE_NAME, FUN_NAME, "failed accept com");
            return NULL;
        }
        okServer("accepteUser");
        pthread_create(&num_t, NULL, startConnection, temp);
    }
    
    listAdd(thread, pthread_self());
    pthread_kill(nb_main, SIGUSR1);
    printf("je meur accepteUser\n");
    return NULL;
}

static void clearTls(void* infos) {
    (void)infos;
}

void *requestHandler(void *arg) {
    char *FUN_NAME = "requestHandler";
    (void)arg;
    TLS_error error;
    Packet *packet;
    TLS_infos *temp;
    Client *c;
    GenList *tls;
    unsigned int i;

    tls = initGenList(clientListSize(user));
    while (!end) {
        genListClear(tls, clearTls);
        for(i = 0; i < clientListSize(user); i++) {
            c = clientListGet(user, i);
            genListAdd(tls, c->info_user);
        }
        switch(tlsWaitOnMultiple(tls, 10)) {
            case TLS_RETRY:
                continue;
                break;
            case TLS_ERROR:
            case TLS_CLOSE:
            case TLS_NULL_POINTER:
                warnl(FILE_NAME, FUN_NAME, "tlsWaitOnMultiple failed");
                end = true;
                continue;
                break;
            case TLS_SUCCESS:
                break;
        }
        for (i = 0; i < clientListSize(user); i++) {
            error = TLS_RETRY;
            c = clientListGet(user, i);
            
            temp = c->info_user;
            packet = NULL;
            if(c->etat != TRY_CONNECTION){
                error = tlsReceiveNonBlocking(temp, &packet);
            }
            switch (error) {
            case TLS_SUCCESS:
                tryServer("new request in requestHandler");
                if (packet->type == PACKET_P2P_MSG) {
                    switch (packet->p2p.type) {
                    case P2P_ACCEPT:
                        acceptHandler(packet, c);
                        break;
                    case P2P_REJECT:
                        rejecttHandler(packet, c);
                        break;
                    case P2P_REQUEST_OUT:
                        requestP2PtHandler(packet, c);
                        break;
                    case P2P_GET_AVAILABLE:
                        getAvailableHandler(packet, c);
                        break;
                    default:
                        break;
                    }
                }
                break;
            case TLS_CLOSE:
                tryServer("TLS_CLOSE");
                clientListDelete(user, i);
                printf("user disconneted\n");
                break;
            case TLS_NULL_POINTER:
                clientListRemove(user, i);
                break;
            case TLS_ERROR:
                tryServer("TLS_CLOSE");
                clientListDelete(user, i);
                printf("user disconneted\n");
                break;
            case TLS_RETRY:
                break;
            }
        }
    }
    listAdd(thread, pthread_self());
    pthread_kill(nb_main, SIGUSR1);
    printf("je meurt requestHandler\n");
    return NULL;
}

bool genListContainsString(GenList *l, char *name) {
    char *temp;
    for (unsigned int i = 0; i < genListSize(l); i++){
        temp = genListGet(l, i);
        if (strcmp(temp, name) == 0) {
            return true;
        }
    }
    return false;
}