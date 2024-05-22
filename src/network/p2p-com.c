#include <bits/types/struct_timeval.h>
#include <network/manager.h>
#include <network/p2p-com.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_P2P_COM "p2p-com.c"
#define TIMEOUT_DIRECT_SERVER 2
#define TIMEOUT_DIRECT_CLIENT 1
#define P2P_PRIVATE_IP "127.0.0.1"
#define P2P_PRIVATE_PORT 7000
#define P2P_PUBLIC_PORT -1

#define CLIENT_CERT_PATH                                                                           \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define CLIENT_KEY_PATH                                                                            \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

typedef struct s_p2p_thread_args {
    Manager *manager;
    TLS_infos *tls;
    TLS_mode mode;
} P2P_thread_args;

void exitThreadPeer(P2P_thread_args *args) {
    managerSetState(args->manager, MANAGER_MOD_PEER, MANAGER_STATE_CLOSED);
    managerMainSendPthreadToJoin(args->manager, pthread_self());
    if (args->tls)
        deinitTLSInfos(&(args->tls));
    free(args);
    pthread_exit(NULL);
}

TLS_error p2pTryTonConnect(Manager *manager, TLS_infos *tls, TLS_mode mode, bool loop) {
    char *FUN_NAME = "p2pTryTonConnect";
    assertl(manager, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "manager NULL");
    assertl(tls, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "tls NULL");

    TLS_error tls_error;
    Manager_error manager_error;
    Packet *packet;
    struct timeval *timeout;

    /* set timeout */
    timeout = malloc(sizeof(struct timeval));
    timeout->tv_sec = (mode == TLS_CLIENT) ? (TIMEOUT_DIRECT_CLIENT) : (TIMEOUT_DIRECT_SERVER);

    /* loop try to connect */
    do {
        tls_error = tlsOpenCom(tls, timeout);
        /* try to connect */
        switch (tls_error) {
        case TLS_SUCCESS:
            return TLS_SUCCESS;
            break;
        case TLS_RETRY:
            warnl(FILE_P2P_COM, FUN_NAME, "connection timeout");
            break;
        case TLS_ERROR:
            warnl(FILE_P2P_COM, FUN_NAME, "tlsOpenCom failed");
            return TLS_ERROR;
            break;
        default:
            warnl(FILE_P2P_COM, FUN_NAME, "unexcpected error (%d)", tls_error);
            return TLS_ERROR;
            break;
        }

        /* check for close request */

        manager_error = managerReceiveNonBlocking(manager, MANAGER_MOD_PEER, &packet);
        switch (manager_error) {
        case MANAGER_ERR_SUCCESS:
            if (packet->type == PACKET_P2P_MSG && packet->p2p.type == P2P_CLOSE) {
                return TLS_CLOSE;
            } else {
                warnl(FILE_P2P_COM, FUN_NAME, "unexpected packet received");
            }
            break;
        case MANAGER_ERR_RETRY:
            break;
        case MANAGER_ERR_CLOSED:
            warnl(FILE_P2P_COM, FUN_NAME, "%s - manager closed",
                  managerErrorToString(manager_error));
            return TLS_ERROR;
            break;
        case MANAGER_ERR_ERROR:
            warnl(FILE_P2P_COM, FUN_NAME, "%s - manager error",
                  managerErrorToString(manager_error));
            return TLS_ERROR;
            break;
        }

    } while (loop);
    return TLS_ERROR;
}

void *funStartPeerDirect(void *arg) {
    char *FUN_NAME = "funStartPeerDirect";
    P2P_thread_args *thread_args = (P2P_thread_args *)arg;
    assertl(thread_args, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "thread_args NULL");
    assertl(thread_args->tls, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "thread_args->tls NULL");
    assertl(thread_args->manager, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE,
            "thread_args->manager NULL");

    TLS_error tls_error;
    Manager_error manager_error;
    Packet *packet;

    tls_error = p2pTryTonConnect(thread_args->manager, thread_args->tls, thread_args->mode, true);

    if (tls_error != TLS_SUCCESS) {
        exitThreadPeer(thread_args);
    }

    tls_error = tlsStartListenning(thread_args->tls, thread_args->manager, MANAGER_MOD_PEER,
                                   tlsManagerPacketGetNext, tlsManagerPacketReceived);
    switch (tls_error) {
    case TLS_CLOSE:
        break;
    case TLS_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "%s - failure tlsStartListenning",
              tlsErrorToString(tls_error));
        break;
    default:
        warnl(FILE_P2P_COM, FUN_NAME, "%s - tlsStartListenning closed with unexpected error",
              tlsErrorToString(tls_error));
        break;
    }

    deinitTLSInfos(&(thread_args->tls));
    managerSetState(thread_args->manager, MANAGER_MOD_PEER, MANAGER_STATE_CLOSED);
    managerMainSendPthreadToJoin(thread_args->manager, pthread_self());

    return NULL;
}

void *funStartPeer(void *arg) {
    char *FUN_NAME = "funStartPeer";
    P2P_thread_args *thread_args = (P2P_thread_args *)arg;
    assertl(thread_args, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "thread_args NULL");
    assertl(thread_args->tls, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE, "thread_args->tls NULL");
    assertl(thread_args->manager, FILE_P2P_COM, FUN_NAME, EXIT_FAILURE,
            "thread_args->manager NULL");

    Packet *packet;
    P2P_msg *msg;
    Manager_error manager_error;
    TLS_error tls_error;
    TLS_infos *tls;

    /* Waiting ACCEPT / REQUEST from server */
    manager_error = managerReceiveBlocking(thread_args->manager, MANAGER_MOD_PEER, &packet);
    if (manager_error != MANAGER_ERR_SUCCESS) {
        warnl(FILE_P2P_COM, FUN_NAME, "%s - manager failure", managerErrorToString(manager_error));
        exitThreadPeer(thread_args);
    }
    if (packet->type != PACKET_P2P_MSG) {
        warnl(FILE_P2P_COM, FUN_NAME, "unexpected type %s during connection",
              packetTypeToString(packet->type));
        exitThreadPeer(thread_args);
    }
    switch (p2pMsgGetType(&(packet->p2p))) {
    case P2P_GET_INFOS:
        break;
    case P2P_CLOSE:
    case P2P_REJECT:
        exitThreadPeer(thread_args);
        break;
    default:
        warnl(FILE_P2P_COM, FUN_NAME, "unexpected type %s during connection",
              p2pMsgTypeToString(p2pMsgGetType(&(packet->p2p))));
        exitThreadPeer(thread_args);
    }
    deinitPacket(&packet);

    /* send informations */
    msg = initP2PMsg(P2P_INFOS);
    p2pMsgSetPrivateIp(msg, P2P_PRIVATE_IP);
    p2pMsgSetPrivatePort(msg, P2P_PRIVATE_PORT);
    p2pMsgSetPublicPort(msg, P2P_PUBLIC_PORT);
    packet = initPacketP2PMsg(msg);
    manager_error = managerSend(thread_args->manager, MANAGER_MOD_SERVER, packet);
    deinitPacket(&packet);
    deinitP2PMsg(&msg);
    if (manager_error != MANAGER_ERR_SUCCESS) {
        warnl(FILE_P2P_COM, FUN_NAME, "%s - manager failed", managerErrorToString(manager_error));
        exitThreadPeer(thread_args);
    }

    char *ip;
    int port;
    TLS_mode mode;
    while (true) {
        /* receive connection informations */
        manager_error = managerReceiveNonBlocking(thread_args->manager, MANAGER_MOD_PEER, &packet);
        if (manager_error != MANAGER_ERR_SUCCESS) {
            exitThreadPeer(thread_args);
        }

        /* read informations */
        switch (p2pMsgGetType(&(packet->p2p))) {
        case P2P_CLOSE:
        case P2P_REJECT:
            exitThreadPeer(thread_args);
            break;
        case P2P_TRY_CLIENT_MODE:
            ip = p2pMsgGetTryIp(&(packet->p2p));
            port = p2pMsgGetTryPort(&(packet->p2p));
            mode = TLS_CLIENT;
            tls = initTLSInfos(ip, port, mode, NULL, NULL);
            free(ip);
            break;
        case P2P_TRY_SERVER_MODE:
            ip = p2pMsgGetTryIp(&(packet->p2p));
            port = p2pMsgGetTryPort(&(packet->p2p));
            mode = TLS_SERVER;
            tls = initTLSInfos(ip, port, mode, NULL, NULL);
            free(ip);
            break;
        default:
            warnl(FILE_P2P_COM, FUN_NAME, "unexpected type %s during connection",
                  p2pMsgTypeToString(p2pMsgGetType(&(packet->p2p))));
            continue;
        }
        deinitPacket(&packet);

        /* try to open connection */
        if (!tls) {
            warnl(FILE_P2P_COM, FUN_NAME, "failed init TLS_infos");
            exitThreadPeer(thread_args);
        }
        tls_error = p2pTryTonConnect(thread_args->manager, tls, mode, false);
        if (tls_error == TLS_CLOSE) {
            exitThreadPeer(thread_args);
        }
        if (tls_error == TLS_SUCCESS)
            break;
        deinitTLSInfos(&tls);
    }

    tls_error = tlsStartListenning(tls, thread_args->manager, MANAGER_MOD_PEER,
                                   tlsManagerPacketGetNext, tlsManagerPacketReceived);
    switch (tls_error) {
    case TLS_CLOSE:
        break;
    case TLS_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "%s - failure tlsStartListenning",
              tlsErrorToString(tls_error));
        break;
    default:
        warnl(FILE_P2P_COM, FUN_NAME, "%s - tlsStartListenning closed with unexpected error",
              tlsErrorToString(tls_error));
        break;
    }
    deinitTLSInfos(&tls);
    managerSetState(thread_args->manager, MANAGER_MOD_PEER, MANAGER_STATE_CLOSED);
    managerMainSendPthreadToJoin(thread_args->manager, pthread_self());

    return NULL;
}

/**
 * @brief Create Peer thread, init the peer manager and launch the peer start function
 *
 * @param manager Manager
 * @param tls TLS_infos init if direct connection, NULL otherwise
 * @return 0 if success, -1 otherwise
 */
int p2pCreateThreadPeer(Manager *manager, TLS_infos *tls, TLS_mode mode) {
    char FUN_NAME[32] = "";
    Manager_error error;
    pthread_t num_t;
    P2P_thread_args *args;

    /* init manager peer */
    managerSetState(manager, MANAGER_MOD_PEER, MANAGER_STATE_IN_PROGRESS);

    /* create struct P2P_thread_args */
    args = malloc(sizeof(P2P_thread_args));
    args->manager = manager;
    args->tls = tls;
    args->mode = mode;

    if (tls && pthread_create(&num_t, NULL, funStartPeerDirect, args) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to lunch thread with funStartPeerDirect");
        managerSetState(manager, MANAGER_MOD_PEER, MANAGER_STATE_CLOSED);
        return -1;
    } else if (pthread_create(&num_t, NULL, funStartPeer, args) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to lunch thread with funStartPeer");
        managerSetState(manager, MANAGER_MOD_PEER, MANAGER_STATE_CLOSED);
        return -1;
    }
    return 0;
}

void p2pGetlistUsersAvailable(Manager *manager) {
    char FUN_NAME[32] = "p2pGetlistUsersAvailable";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");

    Manager_error error;
    P2P_msg *msg = initP2PMsg(P2P_GET_AVAILABLE);
    Packet *packet = initPacketP2PMsg(msg);

    error = managerSend(manager, MANAGER_MOD_SERVER, packet);
    switch (error) {
    case MANAGER_ERR_SUCCESS:
        break;
    case MANAGER_ERR_CLOSED:
        warnl(FILE_P2P_COM, FUN_NAME, "server manager closed");
        break;
    case MANAGER_ERR_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "failed to send packet to server manager");
        break;
    default:
        break;
    }
    deinitPacket(&packet);
    deinitP2PMsg(&msg);
}

void p2pSendRequestConnection(Manager *manager, char *peer_id) {
    char FUN_NAME[32] = "p2pSendRequestConnection";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");
    assertl(peer_id, FILE_P2P_COM, FUN_NAME, -1, "peer_id NULL");

    if (p2pCreateThreadPeer(manager, NULL, TLS_CLIENT) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to create peer thread");
        return;
    }

    Packet *packet;
    P2P_msg *msg;
    Manager_error error;

    msg = initP2PMsg(P2P_REQUEST_OUT);
    p2pMsgSetUserId(msg, peer_id);
    packet = initPacketP2PMsg(msg);

    error = managerSend(manager, MANAGER_MOD_SERVER, packet);
    switch (error) {
    case MANAGER_ERR_SUCCESS:
        break;
    case MANAGER_ERR_CLOSED:
        warnl(FILE_P2P_COM, FUN_NAME, "server manager closed");
        break;
    case MANAGER_ERR_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "failed to send packet to server manager");
        break;
    default:
        break;
    }
    deinitPacket(&packet);
    deinitP2PMsg(&msg);

    if (error == MANAGER_ERR_ERROR || error == MANAGER_ERR_CLOSED) {
        msg = initP2PMsg(P2P_CLOSE);
        p2pMsgSetError(msg, P2P_ERR_LOCAL_ERROR);
        packet = initPacketP2PMsg(msg);
        managerSend(manager, MANAGER_MOD_PEER, packet);
        deinitPacket(&packet);
        deinitP2PMsg(&msg);
    }
}

void p2pRespondToRequest(Manager *manager, char *peer_id, bool response) {
    char FUN_NAME[32] = "p2pRespondToRequest";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");
    assertl(peer_id, FILE_P2P_COM, FUN_NAME, -1, "peer_id NULL");

    /* create thread peer */
    if (response && p2pCreateThreadPeer(manager, NULL, TLS_CLIENT) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to create peer thread");
        response = false;
    }

    Packet *packet;
    P2P_msg *msg;
    Manager_error error;

    /* send response*/
    msg = (response) ? (initP2PMsg(P2P_ACCEPT)) : (initP2PMsg(P2P_REJECT));
    p2pMsgSetUserId(msg, peer_id);
    packet = initPacketP2PMsg(msg);

    error = managerSend(manager, MANAGER_MOD_SERVER, packet);
    switch (error) {
    case MANAGER_ERR_SUCCESS:
        break;
    case MANAGER_ERR_CLOSED:
        warnl(FILE_P2P_COM, FUN_NAME, "server manager closed");
        break;
    case MANAGER_ERR_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "failed to send packet to server manager");
        break;
    default:
        break;
    }
    deinitPacket(&packet);
    deinitP2PMsg(&msg);

    if (response && (error == MANAGER_ERR_ERROR || error == MANAGER_ERR_CLOSED)) {
        msg = initP2PMsg(P2P_CLOSE);
        p2pMsgSetError(msg, P2P_ERR_LOCAL_ERROR);
        packet = initPacketP2PMsg(msg);
        managerSend(manager, MANAGER_MOD_PEER, packet);
        deinitPacket(&packet);
        deinitP2PMsg(&msg);
    }
}

void p2pStartDirectConnection(Manager *manager, TLS_mode mode, char *ip, int port) {
    char FUN_NAME[32] = "p2pStartDirectConnection";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");
    assertl(ip, FILE_P2P_COM, FUN_NAME, -1, "id_user NULL");

    TLS_infos *tls;

    tls = initTLSInfos(ip, port, mode, CLIENT_CERT_PATH, CLIENT_KEY_PATH);
    if (!tls) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to init tls");
        return;
    }

    if (p2pCreateThreadPeer(manager, tls, mode) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to create peer thread");
        return;
    }
}

void p2pCloseCom(Manager *manager, char *peer_id) {

    char FUN_NAME[32] = "p2pCloseCom";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");
    assertl(peer_id, FILE_P2P_COM, FUN_NAME, -1, "peer_id NULL");

    if (managerGetState(manager, MANAGER_MOD_PEER) == MANAGER_STATE_CLOSED) {
        return;
    }

    Manager_error error;
    P2P_msg *msg = initP2PMsg(P2P_CLOSE);
    p2pMsgSetError(msg, P2P_ERR_USER_CLOSE);
    Packet *packet = initPacketP2PMsg(msg);

    error = managerSend(manager, MANAGER_MOD_PEER, packet);
    switch (error) {
    case MANAGER_ERR_SUCCESS:
        break;
    case MANAGER_ERR_CLOSED:
        warnl(FILE_P2P_COM, FUN_NAME, "peer manager closed");
        break;
    case MANAGER_ERR_ERROR:
        warnl(FILE_P2P_COM, FUN_NAME, "failed to send P2P_CLOSE packet to peer manager");
        break;
    default:
        break;
    }
    deinitPacket(&packet);
    deinitP2PMsg(&msg);
}
