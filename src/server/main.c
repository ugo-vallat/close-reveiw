#include "types/clientlist.h"
#include "utils/config.h"
#include <mysql.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <server/cli.h>
#include <server/database-manager.h>
#include <server/request-handler.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types/genericlist.h>
#include <types/list.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>


#define FILE_NAME "Main"

MYSQL *conn;
ClientList *user;
List *thread;
pthread_t nb_main;
bool end = false;

void signal_handler(int sig) {
    (void)sig;
    pthread_join(listPop(thread), NULL);
    printf("RIP Threads\n");
}

int main(int argc, char *argv[]) {
    char *FUN_NAME = "MAIN";

    // printf("main thread : %d\n", getpid());

    init_logger("logs.log", "server");


    Config_infos *config = loadConfig(SERVER);

    user = initClientList(10);
    thread = initList(10);

    char server[32] = "localhost"; // TODO les faire passer en argument
    char sql_user[32] = "newuser";
    char sql_password[32] = "password";
    char database[32] = "testdb";


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
    TLS_infos *tls = initTLSInfos(NULL, config->server.port, TLS_MAIN_SERVER, config->config_ssl.certificate, config->config_ssl.key);

    if (!tls) {
        warnl("main.c", "main", "fail init TLS info");
        return 1;
    }

    nb_main = pthread_self();

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

    tryServer("mise en place du mask");

    struct sigaction action;

    action.sa_flags = SA_SIGINFO;     
    action.sa_handler = signal_handler;
    sigaction(SIGUSR1, &action, NULL);

    okServer("mask");


    while (true) {
        pause();
    }

    tlsCloseCom(tls, NULL);
    tryServer("main je me suicide...");
    return 0;
}
