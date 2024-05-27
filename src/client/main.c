#include <client/tui.h>
#include <network/manager.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <server/weak_password.h>
#include <stdio.h>
#include <string.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/config.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

#define FILE_MAIN "main.c"

Manager *manager;
TLS_infos *tls;
t_config *config;

void *threadServer(void *arg);

void closeApp();

int main(int argc, char *argv[]) {
    char *FUN_NAME = "main";
    pthread_t num_t;
    bool close = false;
    int error;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    refresh();

    /* init logger */
    if (argc > 2)
        exitl(FILE_MAIN, FUN_NAME, -1, "usage : %s [logger_id]", argv[0]);
    if (argc == 2) {
        init_logger(PATH_LOG, argv[1]);
    } else {
        init_logger(PATH_LOG, NULL);
    }

    /* get server infos */
    config = loadConfig();
    if (config == NULL) {
        warnl(FILE_MAIN, FUN_NAME, "failed to load config");
        closeApp();
    }
    if (config->server.is_defined == false) {
        warnl(FILE_MAIN, FUN_NAME, "server address/port undefined");
        closeApp();
    }

    /* create manager */
    manager = initManager();
    if (!manager) {
        exitl(FILE_MAIN, FUN_NAME, -1, "failed init manager");
    }

    /* set manager in progress */
    managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_IN_PROGRESS);
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_IN_PROGRESS);
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_IN_PROGRESS);

    /* creation threads */
    Manager_state state;
    if (pthread_create(&num_t, NULL, threadServer, NULL) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread server");
        closeApp();
    } else {

        while ((state = managerGetState(manager, MANAGER_MOD_SERVER)) == MANAGER_STATE_IN_PROGRESS)
            ;
        if (state == MANAGER_STATE_CLOSED) {
            pthread_join(num_t, NULL);
            closeApp();
        }
    }
    if (pthread_create(&num_t, NULL, stdinHandler, manager) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread input");
        closeApp();
    }
    if (pthread_create(&num_t, NULL, stdoutHandler, manager) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread output");
        closeApp();
    }

    printl("%s === App Started === %s", BLUE, RESET);

    /* wait threads */
    managerSetState(manager, MANAGER_MOD_MAIN, MANAGER_STATE_OPEN);
    while (!close) {
        managerMainReceive(manager, &num_t);
        error = pthread_join(num_t, NULL);
        if (error) {
            warnl(FILE_MAIN, FUN_NAME, "failed to join thread %lu : error %d ", num_t, error);
        }
        close = !isManagerModuleOpen(manager);
    }

    /* close app */
    close_logger();
    closeApp();
    return 1;
}

/**
 * @brief Launch thread server
 *
 * @param arg Ignored
 * @return NULL
 * @note set Manager state to OPEN if ready, CLOSED otherwise
 */
void *threadServer(void *arg) {
    char *FUN_NAME = "threadServer";
    assertl(manager, FILE_MAIN, FUN_NAME, -2, "manager closed");
    TLS_error tls_error;
    pthread_t num_t;

    /* open connection server */
    tls = initTLSInfos(config->server.ip, config->server.port, TLS_CLIENT, NULL, NULL);
    if (!tls) {
        warnl(FILE_MAIN, FUN_NAME, "failed to init tls infos");
        managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_CLOSED);
        return NULL;
    }
    tls_error = tlsOpenCom(tls, NULL);
    if (tls_error != TLS_SUCCESS) {
        warnl(FILE_MAIN, FUN_NAME, "failed to open com with server");
        managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_CLOSED);
        return NULL;
    }
    managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_OPEN);

    /* start listenning */
    tls_error = tlsStartListenning(tls, manager, MANAGER_MOD_SERVER, tlsManagerPacketGetNext, tlsManagerPacketReceived);
    switch (tls_error) {
    case TLS_SUCCESS:
    case TLS_CLOSE:
        printl("threadServer > com server closed");
        break;
    default:
        warnl(FILE_MAIN, FUN_NAME, "tlsStartListenning failed with %s", tlsErrorToString(tls_error));
        break;
    }
    managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_CLOSED);
    num_t = pthread_self();
    managerMainSendPthreadToJoin(manager, num_t);
    return NULL;
}

/**
 * @brief deinit all structures off the main
 *
 */
void closeApp() {
    if (tls) {
        deinitTLSInfos(&tls);
    }
    if (manager) {
        deinitManager(&manager);
    }
    if (config) {
        deinitConfig(&config);
    }
    printl("%s === Application closed === %s", BLUE, RESET);
    endwin();
    exit(0);
}
