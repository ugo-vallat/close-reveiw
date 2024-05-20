#ifndef __P2P_MSG_H__
#define __P2P_MSG_H__

#include <types/genericlist.h>
#include <utils/const-define.h>

typedef enum e_p2p_msg_type {
    /* connection server */
    P2P_CONNECTION_SERVER,
    P2P_CONNECTION_OK,
    P2P_CONNECTION_KO,

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

    /* close connection */
    P2P_CLOSE
} P2P_msg_type;

typedef enum e_p2p_error {
    P2P_ERR_SUCCESS,
    P2P_ERR_UNKNOWN_USER,
    P2P_ERR_UNAVAILABLE_USER,
    P2P_ERR_USER_DISCONNECTED,
    P2P_ERR_ALREADY_PEERING,
    P2P_ERR_USER_CLOSE,
    P2P_ERR_LOCAL_ERROR,
    P2P_ERR_CONNECTION_FAILED,
    P2P_ERR_OTHER
} P2P_error;

typedef struct s_p2p_msg {
    P2P_msg_type type;

    /* id */
    char user_id[SIZE_NAME];
    char user_password[SIZE_PASSWORD];

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

    /* error message */
    P2P_error error;
} P2P_msg;

/**
 * @brief Init struct P2P_msg
 *
 * @param[out] msg P2P_msg* to init
 * @param[in] type Type of the message
 * @return O if success, -1 otherwise
 */
P2P_msg *initP2PMsg(P2P_msg_type type);

/**
 * @brief Deletes struct P2P_msg and free the memory
 *
 * @param msg P2P_msg to delete
 */
void deinitP2PMsg(P2P_msg **msg);

/**
 * @brief Convert P2P_msg into char*
 *
 * @param[in] msg P2P_msg to convert
 * @return char*
 * @note max size = SIZE_TXT
 */
char *p2pMsgToTXT(P2P_msg *msg);

/**
 * @brief  Convert P2P_msg into char* and store it in txt
 *
 * @param[in] msg P2P_msg to convert
 * @param[out] txt TXT buffer
 * @note max size = SIZE_TXT
 */
void p2pMsgIntoTXT(P2P_msg *msg, char *txt);

/**
 * @brief Copy msg_src into msg_dst
 *
 * @param msg_dst Source message
 * @param msg_src Destination message
 */
void p2pMsgCopy(P2P_msg *msg_dst, P2P_msg *msg_src);

/*
    Getteur on P2P_msg*
*/

P2P_msg_type p2pMsgGetType(P2P_msg *msg);

char *p2pMsgGetUserId(P2P_msg *msg);

char *p2pMsgGetPassword(P2P_msg *msg);

GenList *p2pMsgGetListUserOnline(P2P_msg *msg);

int p2pMsgGetPublicPort(P2P_msg *msg);

int p2pMsgGetPrivatePort(P2P_msg *msg);

char *p2pMsgGetPrivateIp(P2P_msg *msg);

char *p2pMsgGetTryIp(P2P_msg *msg);

int p2pMsgGetTryPort(P2P_msg *msg);

P2P_error p2pMsgGetError(P2P_msg *msg);

/*
    Setteur on P2P_msg*
*/
void p2pMsgSetType(P2P_msg *msg, P2P_msg_type type);

void p2pMsgSetUserId(P2P_msg *msg, char *user_id);

void p2pMsgSetPassword(P2P_msg *msg, char *password);

void p2pMsgSetListUserOnline(P2P_msg *msg, GenList *list_online);

void p2pMsgSetPublicPort(P2P_msg *msg, int port);

void p2pMsgSetPrivatePort(P2P_msg *msg, int port);

void p2pMsgSetPrivateIp(P2P_msg *msg, char *ip);

void p2pMsgSetTryInfo(P2P_msg *msg, char *ip, int port);

void p2pMsgSetError(P2P_msg *msg, P2P_error);

/*
    debug
*/

void p2pMsgPrintl(P2P_msg *msg);

#endif