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
#include <utils/logger.h>
#include <utils/project_constants.h>

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

#define PATH_CONFIG_FILE "path/to/config/file"

#define FILE_MAIN "main.c"

Manager *manager;
TLS_infos *tls;
char server_ip[SIZE_IP_CHAR];
int server_port;

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

    /* logger */
    init_logger("logs.log");

    /* get server infos */
    // temporary
    if (argc != 3)
        exitl(FILE_MAIN, FUN_NAME, -1, "usage : %s <server ip> <server port>", argv[0]);
    strncpy(server_ip, argv[1], SIZE_IP_CHAR);
    server_port = atoi(argv[2]);

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

    printf("\n%s === App Started ===%s\n\n", BLUE, RESET);

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
    tls = initTLSInfos(server_ip, server_port, TLS_CLIENT, NULL, NULL);
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
    printf("\n%s === Application closed === %s\n\n", BLUE, RESET);
    endwin();
    exit(0);
}
