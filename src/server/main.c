#include "types/p2p-msg.h"
#include "types/packet.h"
#include <network/tls-com.h>
#include <pthread.h>
#include <server/database-manager.h>
#include <types/genericlist.h>
#include <utils/logger.h>

MYSQL *conn;
GenList *user;
GenList *thread;

char FILE_NAME[16] = "MAIN";

void *startConnection(void *arg) {
    TLS_infos *temp = arg;

    Packet *receive;
    Packet *send;
    P2P_msg *msg_send;
    TLS_error error = TLS_RETRY;
    while (TLS_RETRY) {
        error = tlsReceiveBlocking(temp, &receive);
    }

    if (error == TLS_SUCCESS) {
        if (receive->type == PACKET_P2P_MSG) {
            P2P_msg msg = receive->p2p;
            if (msg.type == P2P_CONNECTION_SERVER) {
                if (login(conn, msg.user_id, msg.user_password)) {
                    genListAdd(user, temp);
                    msg_send = initP2PMsg(P2P_CONNECTION_OK);
                    p2pMsgSetError(msg_send, P2P_ERR_SUCCESS);
                } else {
                    initP2PMsg(P2P_CONNECTION_KO);
                    p2pMsgSetError(msg_send, P2P_ERR_CONNECTION_FAILED);
                }
                send = initPacketP2PMsg(msg_send);
                tlsSend(temp, send);
            }
        }
    }
}

// void accepteUser(GenList *users, TLS_infos *tsl,MYSQL *conn){

//     TLS_infos *temp;

//     unsigned long nbt;

//     while(true){
//         temp = tlsAcceptCom(tsl);
//         pthread_create(&nbt, NULL, startConnection,temp);
//     }

// }

int main() {
    char *FUN_NAME = "MAIN";
    init_logger(NULL);

    MYSQL_RES *res;
    MYSQL_ROW row;

    user = initGenList(10);

    char *server = "localhost"; // TODO les faire passer en argument
    char *sql_user = "newuser";
    char *sql_password = "password";
    char *database = "";

    char *path_cert;
    char *path_key;

    char *ip_server;
    int port_server;

    // TODO modifier pour que se soit dans u scripte appart
    /* Initialisation de la connexion à la base de données */
    conn = mysql_init(NULL);
    if (conn == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }
    /* Connexion à la base de données */
    if (mysql_real_connect(conn, server, sql_user, sql_password, database, 0, NULL, 0) == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }

    TLS_infos *tls = initTLSInfos(ip_server, port_server, TLS_MAIN_SERVER, path_cert, path_key);

    if (!tls) {
        warnl("main.c", "main", "fail init TLS info");
        return 1;
    }
}