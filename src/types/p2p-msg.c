#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types/p2p-msg.h>
#include <utils/logger.h>
#include <utils/project_constants.h>


P2P_msg *initP2PMsg(P2P_msg_type type, char *sender) {
        ASSERTL(sender,-1, "sender NULL")
    P2P_msg *msg = malloc(sizeof(P2P_msg));
    memset(msg, 0, sizeof(P2P_msg));
    msg->type = type;
    msg->nb_user_online = 0;
    msg->private_port = -1;
    msg->public_port = -1;
    msg->try_port = -1;
    strncpy(msg->sender_id, sender, SIZE_NAME);
    return msg;
}

void deinitP2PMsg(P2P_msg **msg) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(*msg,-1, "*msg NULL")

    free(*msg);
    *msg = NULL;
}

char *p2pMsgToTXT(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    char *txt = malloc(SIZE_TXT);
    memset(txt, 0, SIZE_TXT);
    switch (msg->type) {
    case P2P_CONNECTION_OK:
        snprintf(txt, SIZE_TXT, "[ INFOS ] P2P_CONNECTION_OK\n");
        break;
    case P2P_CONNECTION_KO:
        snprintf(txt, SIZE_TXT, "[ INFOS ] P2P_CONNECTION_KO\n");
        break;
    case P2P_ACCEPT:
        snprintf(txt, SIZE_TXT, "[ INFOS ] Connection to %s accepted\n", msg->sender_id);
        break;
    case P2P_REJECT:
        snprintf(txt, SIZE_TXT, "[ INFOS ] Connection to %s rejected\n", msg->sender_id);
        break;
    case P2P_REQUEST_IN:
        snprintf(txt, SIZE_TXT, "[ INFOS ] Connection request from %s ? [Accept / reject]\n", msg->peer_id);
        break;
    case P2P_AVAILABLE:
        if (msg->nb_user_online <= 0) {
            snprintf(txt, SIZE_TXT, "[ INFOS ] No users availables\n");
            break;
        }
        char tmp[SIZE_TXT];
        snprintf(txt, SIZE_TXT, "[ INFOS ] Users availables :\n");
        for (unsigned i = 0; i < msg->nb_user_online; i++) {
            snprintf(tmp, SIZE_TXT, "%s - %s\n", txt, msg->list_user_online[i]);
            strncpy(txt, tmp, SIZE_TXT);
        }
        break;
    default:
        snprintf(txt, SIZE_TXT, "[ INFOS ] %s\n", p2pMsgTypeToString(msg->type));
    }
    return txt;
}

void p2pMsgIntoTXT(P2P_msg *msg, char *txt) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(txt,-1, "txt NULL")

    char *TXT = p2pMsgToTXT(msg);
    strncpy(txt, TXT, SIZE_TXT);
    free(TXT);
}

void p2pMsgCopy(P2P_msg *msg_dst, P2P_msg *msg_src) {
        ASSERTL(msg_src,-1, "msg_src NULL")
    ASSERTL(msg_dst,-1, "msg_dst NULL")

    memcpy(msg_dst, msg_src, sizeof(P2P_msg));
}

char *p2pMsgTypeToString(P2P_msg_type type) {
    switch (type) {
    case P2P_CONNECTION_SERVER:
        return "P2P_USER_ID";
    case P2P_CONNECTION_OK:
        return "P2P_CONNECTION_OK";
    case P2P_CONNECTION_KO:
        return "P2P_CONNECTION_KO";
    case P2P_ACCEPT:
        return "P2P_ACCEPT";
    case P2P_REJECT:
        return ("P2P_REJECT");
    case P2P_REQUEST_IN:
        return ("P2P_REQUEST_IN");
    case P2P_REQUEST_OUT:
        return ("P2P_REQUEST_OUT");
    case P2P_GET_AVAILABLE:
        return ("P2P_GET_AVAILABLE");
    case P2P_AVAILABLE:
        return ("P2P_AVAILABLE");
    case P2P_CON_FAILURE:
        return ("P2P_CON_FAILURE");
    case P2P_CON_SUCCESS:
        return ("P2P_CON_SUCCESS");
    case P2P_GET_INFOS:
        return ("P2P_GET_INFOS");
    case P2P_INFOS:
        return ("P2P_INFOS");
    case P2P_TRY_CLIENT_MODE:
        return ("P2P_TRY_CLIENT_MODE");
    case P2P_TRY_SERVER_MODE:
        return ("P2P_TRY_SERVER_MODE");
    case P2P_CLOSE:
        return ("P2P_CLOSE");
    }
}

char *p2pErrorToString(P2P_error error) {
    switch (error) {
    case P2P_ERR_SUCCESS:
        return "P2P_ERR_SUCCESS";
    case P2P_ERR_UNKNOWN_USER:
        return "P2P_ERR_UNKNOWN_USER";
    case P2P_ERR_UNAVAILABLE_USER:
        return "P2P_ERR_UNAVAILABLE_USER";
    case P2P_ERR_USER_DISCONNECTED:
        return "P2P_ERR_USER_DISCONNECTED";
    case P2P_ERR_USER_CLOSE:
        return "P2P_ERR_USER_CLOSE";
    case P2P_ERR_LOCAL_ERROR:
        return "P2P_ERR_LOCAL_ERROR";
    case P2P_ERR_CONNECTION_FAILED:
        return "P2P_ERR_CONNECTION_FAILED";
    case P2P_ERR_OTHER:
        return "P2P_ERR_OTHER";
    }
}

/*
    Getters on P2P_msg
*/

P2P_msg_type p2pMsgGetType(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    return msg->type;
}

char *p2pMsgGetSenderId(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    char *id = malloc(SIZE_NAME);
    strncpy(id, msg->sender_id, SIZE_NAME);
    return id;
}

char *p2pMsgGetPeerId(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    char *id = malloc(SIZE_NAME);
    strncpy(id, msg->peer_id, SIZE_NAME);
    return id;
}

char *p2pMsgGetPasswordHash(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    char *password_hash = malloc(SIZE_HASH);
    strncpy(password_hash, msg->password_hash, SIZE_HASH);
    return password_hash;
}

GenList *p2pMsgGetListUserOnline(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    GenList *l = initGenList(MAX_ONLINE);
    char *id;
    for (unsigned i = 0; i < msg->nb_user_online; i++) {
        id = malloc(SIZE_NAME);
        strncpy(id, msg->list_user_online[i], SIZE_NAME);
        genListAdd(l, id);
    }
    return l;
}

int p2pMsgGetPublicPort(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    return msg->public_port;
}

int p2pMsgGetPrivatePort(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    return msg->private_port;
}

char *p2pMsgGetPrivateIp(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    char *ip = malloc(SIZE_IP_CHAR);
    strncpy(ip, msg->private_ip, SIZE_IP_CHAR);
    return ip;
}

char *p2pMsgGetTryIp(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    char *ip = malloc(SIZE_IP_CHAR);
    strncpy(ip, msg->try_ip, SIZE_IP_CHAR);
    return ip;
}

int p2pMsgGetTryPort(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    return msg->try_port;
}

P2P_error p2pMsgGetError(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")
    return msg->error;
}

/*
    Setteur on P2P_msg*
*/
void p2pMsgSetType(P2P_msg *msg, P2P_msg_type type) {
        ASSERTL(msg,-1, "msg NULL")
    msg->type = type;
}

void p2pMsgSetSenderId(P2P_msg *msg, char *user_id) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(user_id,-1, "usze_id NULL")

    strncpy(msg->sender_id, user_id, SIZE_NAME);
}

void p2pMsgSetPeerId(P2P_msg *msg, char *user_id) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(user_id,-1, "usze_id NULL")

    strncpy(msg->peer_id, user_id, SIZE_NAME);
}

void p2pMsgSetPasswordHash(P2P_msg *msg, char *password_hash) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(password_hash,-1, "password NULL")

    strncpy(msg->password_hash, password_hash, SIZE_HASH);
}

void p2pMsgSetListUserOnline(P2P_msg *msg, GenList *list_online) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(list_online,-1, "list_online NULL")
    char *id;
    msg->nb_user_online = 0;
    for (unsigned i = 0; i < genListSize(list_online) && i < MAX_ONLINE; i++) {
        strncpy(msg->list_user_online[i], genListGet(list_online, i), SIZE_NAME);
        msg->nb_user_online++;
    }
}

void p2pMsgSetPublicPort(P2P_msg *msg, int port) {
        ASSERTL(msg,-1, "msg NULL")

    msg->public_port = port;
}

void p2pMsgSetPrivatePort(P2P_msg *msg, int port) {
        ASSERTL(msg,-1, "msg NULL")

    msg->private_port = port;
}

void p2pMsgSetPrivateIp(P2P_msg *msg, char *ip) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(ip,-1, "ip NULL")

    strncpy(msg->private_ip, ip, SIZE_IP_CHAR);
}

void p2pMsgSetTryInfo(P2P_msg *msg, char *ip, int port) {
        ASSERTL(msg,-1, "msg NULL")
    ASSERTL(ip,-1, "ip NULL")
    strncpy(msg->try_ip, ip, SIZE_IP_CHAR);
    msg->try_port = port;
}

void p2pMsgSetError(P2P_msg *msg, P2P_error error) {
        ASSERTL(msg,-1, "msg NULL")
    msg->error = error;
}

/*
    debug
*/

void p2pMsgPrintl(P2P_msg *msg) {
    printl("\n| type : <%s>", p2pMsgTypeToString(msg->type));
    printl("| sender_id : <%s>", msg->sender_id);
    printl("| peer_id : <%s>", msg->peer_id);
    printl("| nb_user_online : <%d>", msg->nb_user_online);
    for (unsigned i = 0; i < msg->nb_user_online; i++) {
        printl("  | <%s>", msg->list_user_online[i]);
    }
    printl("| public_port : <%d>", msg->public_port);
    printl("| private_port : <%d>", msg->private_port);
    printl("| private_ip : <%s>", msg->private_ip);
    printl("| try_port : <%d>", msg->try_port);
    printl("| try_ip : <%s>", msg->try_ip);
}
