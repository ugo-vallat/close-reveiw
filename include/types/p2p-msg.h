#ifndef __P2P_MSG_H__
#define __P2P_MSG_H__

#include <types/genericlist.h>
#include <utils/const-define.h>

typedef enum e_p2p_msg_type {
    /* connection server */
    P2P_USER_ID,

    /* Connection p2p init */
    P2P_ACCEPT,
    P2P_REJECT,
    P2P_REQUEST_IN,
    P2P_REQUEST_OUT,
    P2P_GET_AVAILABLE,
    P2P_AVAILABLE,

    /* send informations */
    P2P_GET_INFOS,
    P2P_INFOS,

    /* Method to try */
    P2P_CON_SUCCESS,
    P2P_CON_FAILURE,
    P2P_TRY_SERVER_MODE,
    P2P_TRY_CLIENT_MODE,

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
 * @brief Init struct P2P_msg
 *
 * @param msg P2P_msg* to init
 * @param type Type of the message
 * @return O if success, -1 otherwise
 */
int initP2PMsg(P2P_msg *msg, P2P_msg_type type);

/**
 * @brief Copy the message in a new structure
 *
 * @param[in] msg Message to copy
 * @return P2P_msg *
 */
P2P_msg *p2pCopyMsg(P2P_msg *msg);

/**
 * @brief Convert P2P_msg into char*
 *
 * @param msg P2P_msg to convert
 * @return char*
 * @note max size = SIZE_TXT
 */
char *p2pMsgToTXT(P2P_msg *msg);

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