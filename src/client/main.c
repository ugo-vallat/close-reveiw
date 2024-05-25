
#include <bits/pthreadtypes.h>
#include <client/tui.h>
#include <network/manager.h>
#include <network/tls-com.h>
#include <pthread.h>
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

void connectToServer();

void *threadInput(void *arg);
void *threadOutput(void *arg);
void *threadServer(void *arg);

void closeApp();

int main(int argc, char *argv[]) {
    char *FUN_NAME = "main";
    pthread_t num_t;
    bool close = false;
    int error;

    /* logger */
    init_logger(NULL);

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

    /* connection to server*/
    connectToServer();

    /* set manager in progress */
    managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_IN_PROGRESS);
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_IN_PROGRESS);
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_IN_PROGRESS);

    /* creation threads */
    if (pthread_create(&num_t, NULL, threadServer, NULL) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread server");
        closeApp();
    }
    if (pthread_create(&num_t, NULL, stdinHandler, manager) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread input");
        closeApp();
    }
    if (pthread_create(&num_t, NULL, stdoutHandler, manager) != 0) {
        warnl(FILE_MAIN, FUN_NAME, "fialed create thread output");
        closeApp();
    }

    /* set manager open */
    managerSetState(manager, MANAGER_MOD_SERVER, MANAGER_STATE_OPEN);
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_OPEN);
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_OPEN);

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

    return 0;
}

void connectToServer() {
    char *FUN_NAME = "connectToServer";
    P2P_error p2p_error = P2P_ERR_CONNECTION_FAILED;
    char user_id[SIZE_NAME];
    char password[SIZE_PASSWORD];
    char *buffer;
    Packet *packet;
    P2P_msg *p2p;

    /* init tls infos */
    tls = initTLSInfos(server_ip, server_port, TLS_CLIENT, NULL, NULL);
    if (!tls) {
        warnl(FILE_MAIN, FUN_NAME, "failed to init tls infos");
        closeApp();
    }

    /* open communication tls server */
    // if (tlsOpenCom(tls, NULL) != TLS_SUCCESS) {
    //     warnl(FILE_MAIN, FUN_NAME, "failed to connect to server");
    //     closeApp();
    // }

    /* connect client */

    while (p2p_error != P2P_ERR_SUCCESS) {
        /* get user id */
        printf("user id : ");
        if (stdinGetUserInput(&buffer) != TUI_SUCCESS) {
            warnl(FILE_MAIN, FUN_NAME, "failed to read input");
            continue;
        }
        sscanf(buffer, "%s", user_id);
        free(buffer);

        /* get password */
        printf("password : ");
        if (stdinGetUserInput(&buffer) != TUI_SUCCESS) {
            warnl(FILE_MAIN, FUN_NAME, "failed to read input");
            continue;
        }
        sscanf(buffer, "%s", password);
        free(buffer);

        /* send request */
        p2p = initP2PMsg(P2P_CONNECTION_SERVER);
        p2pMsgSetUserId(p2p, user_id);
        p2pMsgSetPassword(p2p, password);
        packet = initPacketP2PMsg(p2p);
        if (tlsSend(tls, packet) != TLS_SUCCESS) {
            warnl(FILE_MAIN, FUN_NAME, "failed to send request to server");
            deinitPacket(&packet);
            deinitP2PMsg(&p2p);
            closeApp();
        }
        deinitPacket(&packet);
        deinitP2PMsg(&p2p);

        /* wait answer */
        if (tlsReceiveBlocking(tls, &packet) != TLS_SUCCESS) {
            warnl(FILE_MAIN, FUN_NAME, "failed to receive answer from server");
            deinitPacket(&packet);
            deinitP2PMsg(&p2p);
            closeApp();
        }

        /* check answer */
        if (packet->type != PACKET_P2P_MSG) {
            warnl(FILE_MAIN, FUN_NAME, "unexpected packet received (%s)", packetTypeToString(packet->type));
        }
        p2p_error = p2pMsgGetError(&(packet->p2p));
        deinitPacket(&packet);
        if (p2p_error != P2P_ERR_SUCCESS) {
            printf("%s > failled connection : %s %s\n", RED, p2pErrorToString(p2p_error), RESET);
        } else {
            printf(" %s> connection succeed %s\n", GREEN, RESET);
        }
    }

    managerSetUser(manager, user_id);
}

void *threadServer(void *arg) {
    char *FUN_NAME = "threadServer";
    assertl(tls, FILE_MAIN, FUN_NAME, -1, "tls closed");
    assertl(manager, FILE_MAIN, FUN_NAME, -2, "manager closed");
    TLS_error tls_error;
    pthread_t num_t;

    tls_error = tlsStartListenning(tls, manager, MANAGER_MOD_SERVER, tlsManagerPacketGetNext, tlsManagerPacketReceived);
    switch (tls_error) {
    case TLS_SUCCESS:
    case TLS_CLOSE:
        printl("com server closed");
        break;
    default:
        warnl(FILE_MAIN, FUN_NAME, "tlsStartListenning failed with %s", tlsErrorToString(tls_error));
        break;
    }
    num_t = pthread_self();
    managerMainSendPthreadToJoin(manager, num_t);
    return NULL;
}

void closeApp() {
    if (tls) {
        deinitTLSInfos(&tls);
    }
    if (manager) {
        deinitManager(&manager);
    }
    printf("\n%s === Application closed === %s\n\n", BLUE, RESET);
    exit(0);
}
