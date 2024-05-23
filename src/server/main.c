#include <mysql.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <server/database-manager.h>
#include <server/request-handler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <types/genericlist.h>
#include <types/list.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>

#define SERVER_CERT_PATH                                                                                               \
    "/home/eritque/Document/BE/close-review/config/server/"                                                            \
    "server-be-auto-cert.crt"
#define SERVER_KEY_PATH "/home/eritque/Document/BE/close-review/config/server/server-be.key"

#define SERVER_PORT 5555

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

MYSQL *conn;
GenList *user;
List *thread;
char FILE_NAME[16] = "MAIN";
bool end = false;

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

void *startConnection(void *arg) {
    TLS_infos *temp = arg;

    Packet *receive;
    Packet *send;
    P2P_msg *msg_send;
    TLS_error error;
    bool connection_etablish = false;
    tryServer("startConnection tlsReceiveBlocking");
    while (!connection_etablish) {
        error = TLS_RETRY;
        while (error == TLS_RETRY) {
            error = tlsReceiveBlocking(temp, &receive);
            printf(" > error : %d\n", error);
            sleep(1);
        }
        tryServer("startConnection check error");
        if (error == TLS_SUCCESS) {
            if (receive->type == PACKET_P2P_MSG) {
                P2P_msg msg = receive->p2p;
                if (msg.type == P2P_CONNECTION_SERVER) {
                    if (login(conn, msg.user_id, msg.user_password, genListSize(user))) {
                        genListAdd(user, temp);
                        msg_send = initP2PMsg(P2P_CONNECTION_OK);
                        p2pMsgSetError(msg_send, P2P_ERR_SUCCESS);
                        printf("user connected\n");
                        okServer("startConnection");
                        connection_etablish = true;
                    } else {
                        msg_send = initP2PMsg(P2P_CONNECTION_KO);
                        p2pMsgSetError(msg_send, P2P_ERR_CONNECTION_FAILED);
                        koServer("startConnection");
                    }
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
    TLS_infos *temp, *tsl = arg;
    pthread_t num_t;
    while (!end) {
        tryServer("accepteUser tlsAcceptCom");
        temp = tlsAcceptCom(tsl);
        okServer("accepteUser");
        pthread_create(&num_t, NULL, startConnection, temp);
    }

    listAdd(thread, pthread_self());
    printf("je meur accepteUser\n");
    return NULL;
}

void *CLI(void *arg) {
    char buff[256];
    scanf("%s", buff);
    end = true;

    listAdd(thread, pthread_self());
    printf("je meur CLI\n");

    
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
                tryServer("TLS_SUCCESS");
                if (packet->type == PACKET_P2P_MSG) {
                    switch (packet->p2p.type) {
                    case P2P_ACCEPT: // TODO
                        break;
                    case P2P_REJECT: // TODO
                        break;
                    case P2P_REQUEST_OUT: // TODO
                        requestP2PtHandler(packet, conn, genListGet(user, i));
                        break;
                    case P2P_GET_AVAILABLE:
                        getAvailableHandler(packet, conn, genListGet(user, i));
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

// int main(void){j
//     char *FUN_NAME = "MAIN";
//     init_logger(NULL);
//     char server[32] = "localhost"; // TODO les faire passer en argument
//     char sql_user[32] = "newuser";
//     char sql_password[32] = "password";
//     char database[32] = "testdb";

//     tryServer("main init SQL");
//     conn = mysql_init(NULL);
//     if (conn == NULL) {
//         exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
//     }
//     /* Connexion à la base de données */
//     if (mysql_real_connect(conn, server, sql_user, sql_password, database, 0, NULL, 0) == NULL) {
//         exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
//     }
//     okServer("main");

//     tryServer("main setup");
//     setup(conn);
//     okServer("main setup");

//     tryServer("main add users");
//     createUser(conn, "ugo", "5cEc56aE8.azdA21");
//     createUser(conn, "coco", "LapouleACOCOdu31");
//     createUser(conn, "jack", "1234");
//     okServer("main");

//     login(conn, "ugo", "5cEc56aE8.azdA21", 0);
//     login(conn, "coco", "LapouleACOCOdu31", 1);
//     login(conn, "jack", "1234", 2);

//     // SQLrequestP2P(conn, "ugo", "coco");
//     // SQLrequestP2P(conn, "jack", "ugo");
//     okServer("FIN");

// }

int main() {
    char *FUN_NAME = "MAIN";

    printf("main thread : %d\n", getpid());

    init_logger(NULL);

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
    createUser(conn, "ugo", "5cEc56aE8.azdA21");
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

    pthread_create(&temp, NULL, CLI, NULL);

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
