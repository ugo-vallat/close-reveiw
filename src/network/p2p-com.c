#include "network/manager.h"
#include <network/p2p-com.h>
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
 * @param ip Ip if direct connection, NULL otherwise
 * @param port Port if direct connection, -1 otherwise
 * @return 0 if success, -1 otherwise
 */
int p2pCreateThreadPeer(Manager *manager, char *ip, int port);

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

void p2pSendRequestConnection(Manager *manager, char *id_peer) {
    char FUN_NAME[32] = "p2pSendRequestConnection";
    assertl(manager, FILE_P2P_COM, FUN_NAME, -1, "manager NULL");
    assertl(id_peer, FILE_P2P_COM, FUN_NAME, -1, "id_peer NULL");

    if (p2pCreateThreadPeer(manager, NULL, -1) != 0) {
        warnl(FILE_P2P_COM, FUN_NAME, "failed to create peer thread");
        return;
    }

    Packet *packet;
    P2P_msg *msg;
    Manager_error error;

    msg = initP2PMsg(P2P_REQUEST_OUT);
    p2pMsgSetUserId(msg, id_peer);
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
        initP2PMsg(P2P_REJECT);
        // TODO
    }
}

/**
 * @brief Send response to the connection request and start peer thread if respond == true
 *
 * @param manager Manager
 * @param id_user User requesting
 * @param respond Response to send
 */
void p2pRespondToRequest(Manager *manager, char *id_user, bool respond);

/**
 * @brief Start direct TLS_connection without passing through the server
 *
 * @param manager Manager
 * @param mode Mode (TLS_CLIENT / TLS_SERVER)
 * @param ip Ip of the peer (ignore if mode = TLS_SERVER)
 * @param port Port of the peer / port to listen
 */
void p2pStartDirectConnection(Manager *manager, TLS_mode mode, char *ip, int port);

/**
 * @brief Close the peer connection with the user peer_id
 *
 * @param manager Manager
 * @param peer_id User connected in P2P
 */
void p2pCloseCom(Manager *manager, char *peer_id);
