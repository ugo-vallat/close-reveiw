#include <mysql.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <server/database-manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types/genericlist.h>
#include <types/list.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>

#define SERVER_CERT_PATH "./config/server/server-be-auto-cert.crt"
#define SERVER_KEY_PATH "./config/server/server-be.key"

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

MYSQL *conn;
GenList *user;
List *thread;
char FILE_NAME[16] = "MAIN";
bool end = false;
int SERVER_PORT;

typedef struct s_tuple_TLS_info {
    TLS_infos *info_user1;
    TLS_infos *info_user2;
} Tuple_TLS_info;

void tryServer(char *arg);

void okServer(char *arg);

void koServer(char *arg);

void acceptHandler(Packet *p, TLS_infos *info, int sender_nb); // TODO

void rejecttHandler(Packet *p, TLS_infos *info);

void requestP2PtHandler(Packet *p, TLS_infos *info);

void getAvailableHandler(Packet *p, TLS_infos *info);

void *startConnection(void *arg);

void *accepteUser(void *arg);

void *requestHandler(void *arg);

int main(int argc, char *argv[]) {
    char *FUN_NAME = "MAIN";

    printf("main thread : %d\n", getpid());

    init_logger(NULL);

    /* get args */
    if (argc != 2) {
        exitl(FILE_NAME, FUN_NAME, -1, "usage : %s <server port>", argv[0]);
    } else {
        SERVER_PORT = atoi(argv[1]);
    }

    user = initGenList(10);
    thread = initList(10);

    char server[32] = "localhost"; // TODO les faire passer en argument
    char sql_user[32] = "newuser";
    char sql_password[32] = "password";
    char database[32] = "testdb";

    char *path_cert;
    char *path_key;

    char *ip_server;
    int port_server;

    // TODO modifier pour que se soit dans u scripte appart
    /* Initialisation de la connexion à la base de données */
    tryServer("main init SQL");
    conn = mysql_init(NULL);
    if (conn == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }
    /* Connexion à la base de données */
    if (mysql_real_connect(conn, server, sql_user, sql_password, database, 0, NULL, 0) == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }
    okServer("main");

    tryServer("main setup");
    setup(conn);
    okServer("main setup");

    tryServer("main add users");
    createUser(conn, "ugo", "1234");
    createUser(conn, "coco", "1234");
    okServer("main");

    tryServer("main init tls");
    TLS_infos *tls = initTLSInfos(NULL, SERVER_PORT, TLS_MAIN_SERVER, SERVER_CERT_PATH, SERVER_KEY_PATH);

    if (!tls) {
        warnl("main.c", "main", "fail init TLS info");
        return 1;
    }

    printf("end initialisation\n");
    okServer("main");
    pthread_t num_t, temp;
    tryServer("main create thread accept");
    pthread_create(&num_t, NULL, accepteUser, tls);
    okServer("main");

    tryServer("main create thread handler");
    pthread_create(&temp, NULL, requestHandler, NULL);
    okServer("main");

    while (!end) {
        while (!listIsEmpty(thread)) {
            pthread_join(listPop(thread), NULL);
            printf("RIP Threads\n");
        }
        sleep(1);
    }

    tlsCloseCom(tls, NULL);
    deinitTLSInfos(&tls);
    pthread_join(num_t, NULL);
    tryServer("main je me suicide...");
    return 0;
}

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
    Tuple_TLS_info *info = arg;
    P2P_msg *msg = initP2PMsg(P2P_GET_INFOS, "serveur");
    Packet *p = initPacketP2PMsg(msg);
    Packet *receive1, *receive2;
    TLS_error error1, error2;

    tlsSend(info->info_user1, p);
    tlsSend(info->info_user2, p);

    error1 = tlsReceiveBlocking(info->info_user1, &receive1);
    error2 = tlsReceiveBlocking(info->info_user2, &receive2);

    msg = initP2PMsg(P2P_TRY_SERVER_MODE, "serveur");

    int try_port = p2pMsgGetPrivatePort(&receive2->p2p);
    char *try_ip = p2pMsgGetPrivateIp(&receive2->p2p);
    p2pMsgSetTryInfo(msg, try_ip, try_port);
    p = initPacketP2PMsg(msg);
    tlsSend(info->info_user1, p);

    msg = initP2PMsg(P2P_TRY_CLIENT_MODE, "serveur");

    try_port = p2pMsgGetPrivatePort(&receive1->p2p);
    try_ip = p2pMsgGetPrivateIp(&receive1->p2p);
    p2pMsgSetTryInfo(msg, try_ip, try_port);
    p = initPacketP2PMsg(msg);
    tlsSend(info->info_user2, p);

    genListAdd(user, info->info_user1);
    genListAdd(user, info->info_user2);
    listAdd(thread, pthread_self());
    return NULL;
}

void acceptHandler(Packet *p, TLS_infos *info, int sender_nb) {
    int target_nb;
    pthread_t temp;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);
    if (SQLaccept(conn, sender, target, &target_nb)) {
        Tuple_TLS_info *info = malloc(sizeof(Tuple_TLS_info));
        info->info_user1 = genListRemove(user, sender_nb);
        info->info_user2 = genListRemove(user, sender_nb);
        pthread_create(&temp, NULL, createP2Pconnection, info);
    }
}

void rejecttHandler(Packet *p, TLS_infos *info) {
    int user_nb;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);
    if (SQLreject(conn, sender, target, &user_nb)) {
        Packet *send;
        P2P_msg *msg;
        msg = initP2PMsg(P2P_REJECT, "serveur");
        p2pMsgSetPeerId(msg, sender);
        send = initPacketP2PMsg(msg);
        tlsSend(genListGet(user, user_nb), send);
        deinitPacket(&send);
        deinitP2PMsg(&msg);
    }
}

void requestP2PtHandler(Packet *p, TLS_infos *info) {
    int user_nb;
    Packet *send;
    P2P_msg *msg;
    char *sender = p2pMsgGetSenderId(&p->p2p);
    char *target = p2pMsgGetPeerId(&p->p2p);
    printl(" > request from <%s> to <%s>", sender, target);
    P2P_error error = SQLrequestP2P(conn, sender, target, &user_nb);

    if (error != P2P_ERR_SUCCESS) {
        printl(" > target unavailable");
        msg = initP2PMsg(P2P_REJECT, "serveur");
        p2pMsgSetError(msg, error);
        send = initPacketP2PMsg(msg);
        tlsSend(info, send);
        deinitPacket(&send);
        deinitP2PMsg(&msg);
    } else {
        printl(" > send request to target");
        msg = initP2PMsg(P2P_REQUEST_IN, "serveur");
        p2pMsgSetPeerId(msg, sender);
        send = initPacketP2PMsg(msg);
        tlsSend(genListGet(user, user_nb), send);
        deinitPacket(&send);
        deinitP2PMsg(&msg);
    }
}

void getAvailableHandler(Packet *p, TLS_infos *info) {
    GenList *res = listUserAvalaible(conn);

    P2P_msg *msg = initP2PMsg(P2P_AVAILABLE, "server");
    p2pMsgSetListUserOnline(msg, res);

    Packet *send = initPacketP2PMsg(msg);

    tlsSend(info, send);

    deinitPacket(&send);
    deinitP2PMsg(&msg);
}

void *startConnection(void *arg) {
    TLS_infos *temp = arg;

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
                    if (login(conn, sender, password_hash, genListSize(user))) {
                        genListAdd(user, temp);
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
            tlsCloseCom(temp, NULL);
            deinitTLSInfos(&temp);
        }
    }
    tryServer("startConnection add thread to list");

    listAdd(thread, pthread_self());

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
            warnl(FILE_NAME, FUN_NAME, "accept accept com");
            return NULL;
        }
        okServer("accepteUser");
        pthread_create(&num_t, NULL, startConnection, temp);
    }

    listAdd(thread, pthread_self());
    printf("je meur accepteUser\n");
    return NULL;
}

void *requestHandler(void *arg) {
    (void)arg;
    TLS_error error;
    Packet *packet;
    TLS_infos *temp;
    while (!end) {
        for (unsigned int i = 0; i < genListSize(user); i++) {
            error = tlsReceiveNonBlocking(genListGet(user, i), &packet);
            switch (error) {
            case TLS_SUCCESS:
                tryServer("new request in requestHandler");
                if (packet->type == PACKET_P2P_MSG) {
                    switch (packet->p2p.type) {
                    case P2P_ACCEPT:
                        acceptHandler(packet, genListGet(user, i), i);
                        break;
                    case P2P_REJECT:
                        rejecttHandler(packet, genListGet(user, i));
                        break;
                    case P2P_REQUEST_OUT:
                        requestP2PtHandler(packet, genListGet(user, i));
                        break;
                    case P2P_GET_AVAILABLE:
                        getAvailableHandler(packet, genListGet(user, i));
                        break;
                    default:
                        break;
                    }
                }
                break;
            case TLS_CLOSE:
                tryServer("TLS_CLOSE");
                temp = genListRemove(user, i);
                tlsCloseCom(temp, NULL);
                deinitTLSInfos(&temp);
                disconnect(conn, i);
                printf("user disconneted\n");
                break;
            case TLS_NULL_POINTER:
                genListRemove(user, i);
                break;
            case TLS_ERROR:
                tryServer("TLS_ERROR");
                temp = genListRemove(user, i);
                tlsCloseCom(temp, NULL);
                deinitTLSInfos(&temp);
                disconnect(conn, i);
                break;
            case TLS_RETRY:
                break;
            }
        }
    }
    listAdd(thread, pthread_self());
    printf("je meur requestHandler\n");
    return NULL;
}
