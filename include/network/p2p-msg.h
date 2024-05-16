#ifndef __P2P_MSG_H__
#define __P2P_MSG_H__

#include "network/tls-com.h"
#include "utils/message.h"

#define MAX_ONLINE 8

typedef enum e_p2p_msg_type {
    /* connection server */
    ID_USER,

    /* Connection p2p init */
    ACCEPT_CONNECTION,
    REJECT_CONNECTION,
    REQUEST_CONNECTION,
    REQUEST_LIST_ONLINE,
    RESPONSE_LIST_ONLINE,
    REQUEST_PEER_REQUEST,
    RESPONSE_PEER_REQUEST,

    /* send informations */
    REQUEST_INFO_COM,
    RESPONSE_INFO_COM,

    /* Method to try */
    CON_SUCCESS,
    CON_FAILURE,
    CON_ABORTED,
    TRY_SERVER_MODE,
    TRY_CLIENT_MODE,

    /* if packet received invalid */
    INVALID_PACKET,
    UNDIFINED_P2P_TYPE
} P2P_msg_type;

typedef struct s_p2p_msg {
    P2P_msg_type type;

    /* id */
    char user_id[SIZE_NAME];

    /* list requests */
    unsigned nb_user_online;
    char list_user_online[MAX_ONLINE][SIZE_NAME];

    /* config info */
    int public_port;
    int private_port;
    char private_ip[SIZE_IP_CHAR];

    /* param com */
    int try_port;
    char try_ip[SIZE_IP_CHAR];
} P2P_msg;

/**
 * @brief Create struct P2P_msg
 *
 * @param[in] type Type of the message
 * @return P2P_msg*
 */
P2P_msg *initP2PMsg(P2P_msg_type type);

/**
 * @brief Delete the message and free memory
 *
 * @param[in] msg Message to delete
 */
void deinitP2PMsg(P2P_msg *msg);

/**
 * @brief Delete P2P_msg passed as a void*
 *
 * @param[in] msg P2P_msg
 */
void deinitP2PMsgGen(void *msg);

/**
 * @brief Copy the message in a new structure
 *
 * @param[in] msg Message to copy
 * @return P2P_msg *
 */
P2P_msg *p2pCopyMsg(P2P_msg *msg);

/*
    Getteur on P2P_msg*
*/
P2P_msg_type p2pMsgGetType(P2P_msg *msg);

char *p2pMsgGetUserId(P2P_msg *msg);

GenList *p2pMsgGetListUserOnline(P2P_msg *msg);

int p2pMsgGetPublicPort(P2P_msg *msg);

int p2pMsgGetPrivatePort(P2P_msg *msg);

char *p2pMsgGetPrivateIp(P2P_msg *msg);

char *p2pMsgGetTryIp(P2P_msg *msg);

int p2pMsgGetTryPort(P2P_msg *msg);

/*
    Setteur on P2P_msg*
*/
void p2pMsgSetType(P2P_msg *msg, P2P_msg_type type);

void p2pMsgSetUserId(P2P_msg *msg, char *user_id);

void p2pMsgSetListUserOnline(P2P_msg *msg, GenList *list_online);

void p2pMsgSetPublicPort(P2P_msg *msg, int port);

void p2pMsgSetPrivatePort(P2P_msg *msg, int port);

void p2pMsgSetPrivateIp(P2P_msg *msg, char *ip);

void p2pMsgSetTryInfo(P2P_msg *msg, char *ip, int port);

/*
    debug
*/

void p2pMsgPrintl(P2P_msg *msg);

#endif