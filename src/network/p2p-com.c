#include <network/manager.h>
#include <network/p2p-com.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <sys/socket.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <utils/const-define.h>
#include <utils/logger.h>

#define FILE_P2P_COM "p2p-com.c"

#define CLIENT_CERT_PATH                                                                           \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define CLIENT_KEY_PATH                                                                            \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

/**
 * @brief Create Peer thread, init the peer manager and launch the peer start function
 *
 * @param manager Manager
 * @param tls TLS_infos init if direct connection, NULL otherwise
 * @return 0 if success, -1 otherwise
 */
int p2pCreateThreadPeer(Manager *manager, TLS_infos *tls);

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

    if (p2pCreateThreadPeer(manager, NULL) != 0) {
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
    if (response && p2pCreateThreadPeer(manager, NULL) != 0) {
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

    if (p2pCreateThreadPeer(manager, tls) != 0) {
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
