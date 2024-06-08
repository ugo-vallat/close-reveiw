#include "types/clientlist.h"
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
#include <server/cli.h>
#include <server/request-handler.h>

#define SERVER_CERT_PATH "./config/server/server-be-auto-cert.crt"
#define SERVER_KEY_PATH "./config/server/server-be.key"

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

#define FILE_NAME "Main"

MYSQL *conn;
ClientList *user;
List *thread;
bool end = false;
int SERVER_PORT;







int main(int argc, char *argv[]) {
    char *FUN_NAME = "MAIN";

    // printf("main thread : %d\n", getpid());

    init_logger("logs.log", "server");

    /* get args */
    if (argc != 2) {
        exitl(FILE_NAME, FUN_NAME, -1, "usage : %s <server port>", argv[0]);
    } else {
        SERVER_PORT = atoi(argv[1]);
    }

    user = initClientList(10);
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
    createUser(conn, "daoud", "1234");
    createUser(conn, "benoit", "1234");
    createUser(conn, "mateo", "1234");
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

    tryServer("main create thread cli");
    pthread_create(&temp, NULL, cli, NULL);
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


