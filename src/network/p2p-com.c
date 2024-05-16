#include "network/gestionary-com.h"
#include "network/p2p-msg.h"
#include "utils/genericlist.h"
#include "utils/logger.h"
#include <network/p2p-com.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <string.h>
#include <utils/message.h>
#include <utils/token.h>

#define LOCAL_IP "127.0.0.1"

#define P2P_SERVER_TIMEOUT 5

#define CLIENT_CERT_PATH                                                                           \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define CLIENT_KEY_PATH                                                                            \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

struct s_P2P_info {
    Gestionary_com *gest;

    /* buffers */
    char id_request_in[SIZE_NAME]; /* request from peer */
};

/**
 * @brief Send list of methods available on this host to establish a P2P connection
 *
 * @param[in] infos TEMPORARY channel with server
 */
void sendMethodsEnable(P2P_infos *infos) {
    P2P_msg *msg = createP2PMsg(RESPONSE_INFO_COM);
    setP2PPrivateIp(msg, LOCAL_IP);
    gestionarySendP2P(infos->gest, msg);
}

/**
 * @brief Setup host as server and wait the remote host to connect
 *
 * @param[in] port Port to bind
 * @return TLS_infos * on success, NULL otherwise
 */
TLS_infos *connectToPeerServerMode(int port) {
    TLS_infos *tls = initTLSInfos("0.0.0.0", port, SERVER, CLIENT_CERT_PATH, CLIENT_KEY_PATH);

    struct timeval timeout;
    timeout.tv_sec = P2P_SERVER_TIMEOUT;
    timeout.tv_usec = 0;
    if (openComTLS(tls, &timeout) != 0) {
        warnl("p2p-com.c", "connectToPeerServerMode", "fail connection");
        deleteTLSInfos(&tls);
        return NULL;
    }
    return tls;
}

/**
 * @brief Setup host as client and try to connect with the remote host
 *
 * @param[in] ip IP of the remote host
 * @param[in] port Port of remote host
 * @return TLS_infos * on success, NULL otherwise
 */
TLS_infos *connectToPeerClientMode(char *ip, int port) {
    TLS_infos *tls = initTLSInfos(ip, port, CLIENT, NULL, NULL);
    if (openComTLS(tls, NULL) != 0) {
        warnl("p2p-com.c", "connectToPeerClientMode", "fail connection");
        deleteTLSInfos(&tls);
        return NULL;
    }
    return tls;
}

Gestionary_com *tryConnectToPeer(P2P_infos *infos) {
    TLS_infos *tls;
    Gestionary_com *gest;
    bool end = false;
    P2P_msg *msg;
    /* send methods */
    sendMethodsEnable(infos);

    /* try connect */
    while (!end) {
        while ((gestionaryReceiveP2P(infos->gest, &msg)) != 1) {
            if (!gestionaryIsComOpen(infos->gest)) {
                warnl("p2p-com.c", "tryConnectToPeer", "com server closed");
                return NULL;
            }
        }

        switch (getP2PType(msg)) {
        case TRY_SERVER_MODE:
            tls = connectToPeerServerMode(getP2PTryPort(msg));
            break;
        case TRY_CLIENT_MODE:
            tls = connectToPeerClientMode(getP2PTryIp(msg), getP2PTryPort(msg));
            break;
        case END:
            end = true;
            break;
        default:
            warnl("p2p-com.c", "tryConnectToPeer", "unvalid type P2P_msg");
            break;
        }
        if (end)
            break;
        deleteP2PMsg(msg);
        if (tls) {
            end = true;
            msg = createP2PMsg(CON_SUCCESS);

        } else {
            msg = createP2PMsg(CON_FAILURE);
        }
        gestionarySendP2P(infos->gest, msg);
    }

    if (!tls) {
        warnl("p2p-com.c", "tryConnectToPeer", "fail connection peer");
        return NULL;
    }
    gest = createGestionaryCom(tls);
    if (!gest) {
        warnl("p2p-com.c", "tryConnectToPeer", "fail open gestionary");
        return NULL;
    }
    return gest;
}

P2P_infos *createP2P_infos(Gestionary_com *gestionary) {
    P2P_infos *infos = malloc(sizeof(P2P_infos));
    infos->gest = gestionary;
    strncpy(infos->id_request_in, "", SIZE_NAME);
    return infos;
}

void deleteP2P_infos(P2P_infos **infos);

GenList *p2pGetListUserConnected(P2P_infos *infos) {
    int ret;
    char *id;
    GenList *list;
    P2P_msg *msg = createP2PMsg(REQUEST_LIST_ONLINE);
    /* send request */
    gestionarySendP2P(infos->gest, msg);
    /* receive response */
    while ((ret = gestionaryReceiveP2P(infos->gest, &msg)) < 1) {
        if (ret < 0 && !gestionaryIsComOpen(infos->gest)) {
            warnl("p2p-com.c", "getListUserConnected", "gestionary closed");
            return NULL;
        }
    }
    if (msg->type != RESPONSE_LIST_ONLINE) {
        warnl("p2p-com.c", "getListUserConnected", "unexpected response from server");
        return NULL;
    }
    /* return list of id */
    list = createGenList(msg->nb_user_online);
    for (unsigned i = 0; i < msg->nb_user_online; i++) {
        id = malloc(SIZE_NAME);
        strncpy(id, msg->list_user_online[i], SIZE_NAME);
        genListAdd(list, id);
    }
    return list;
}

char *p2pGetRequestConnection(P2P_infos *infos) {
    P2P_msg *msg;
    int ret;
    char *id_peer;
    /* send request */
    msg = createP2PMsg(REQUEST_PEER_REQUEST);
    gestionarySendP2P(infos->gest, msg);

    /* wait response */
    while ((ret = gestionaryReceiveP2P(infos->gest, &msg)) < 1) {
        if (ret < 0 && !gestionaryIsComOpen(infos->gest)) {
            warnl("p2p-com.c", "sendRequestP2PConnection", "gestionary closed");
            return NULL;
        }
    }
    if (strncmp(msg->id_peer, "", SIZE_NAME) == 0) {
        return "X";
    }
    id_peer = malloc(SIZE_NAME);
    strncpy(id_peer, msg->id_peer, SIZE_NAME);
    return id_peer;
}

int p2pSendRequestConnection(P2P_infos *infos, char *id_user, Gestionary_com **gest) {
    P2P_msg *msg;
    int ret;
    /* send request */
    msg = createP2PMsg(REQUEST_CONNECTION);
    setP2PIdPeer(msg, id_user);
    gestionarySendP2P(infos->gest, msg);

    /* wait response */
    while ((ret = gestionaryReceiveP2P(infos->gest, &msg)) < 1) {
        if (ret < 0 && !gestionaryIsComOpen(infos->gest)) {
            warnl("p2p-com.c", "sendRequestP2PConnection", "gestionary closed");
            return -1;
        }
    }
    if (msg->type == ACCEPT_CONNECTION) {
        *gest = tryConnectToPeer(infos);
        if (*gest == NULL)
            return 0;
        return 1;
    }
    if (msg->type == REJECT_CONNECTION)
        return 0;
    return -1;
}

int p2pAcceptConnection(P2P_infos *infos, char *id_user, bool accept, Gestionary_com **gest) {
    P2P_msg *msg;
    int ret;
    /* send request */
    if (accept) {
        msg = createP2PMsg(ACCEPT_CONNECTION);
    } else {
        msg = createP2PMsg(REJECT_CONNECTION);
    }
    setP2PIdPeer(msg, id_user);
    gestionarySendP2P(infos->gest, msg);

    if (!accept) {
        return 0;
    }

    /* wait response */
    while ((ret = gestionaryReceiveP2P(infos->gest, &msg)) < 1) {
        if (ret < 0 && !gestionaryIsComOpen(infos->gest)) {
            warnl("p2p-com.c", "sendRequestP2PConnection", "gestionary closed");
            return -1;
        }
    }
    if (msg->type == ACCEPT_CONNECTION) {
        *gest = tryConnectToPeer(infos);
        if (*gest == NULL)
            return 0;
        return 1;
    }
    if (msg->type == REJECT_CONNECTION)
        return 0;
    return -1;
}

int p2pForwardPortWithUpnp(int port_in, int port_out, long time);
