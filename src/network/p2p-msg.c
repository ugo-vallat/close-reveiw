
#include "network/packet.h"
#include "network/tls-com.h"
#include "utils/genericlist.h"
#include "utils/logger.h"
#include "utils/message.h"
#include <network/p2p-msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

P2P_msg *initP2PMsg(P2P_msg_type type) {
    P2P_msg *msg = malloc(sizeof(P2P_msg));
    if (!msg) {
        warnl("p2p-msg.c", "initP2PMsg", "fail malloc P2P_msg");
        return NULL;
    }
    memset(msg, 0, sizeof(P2P_msg));
    msg->nb_user_online = 0;
    msg->private_port = -1;
    msg->public_port = -1;
    msg->try_port = -1;
    msg->type = type;
    return msg;
}

void deinitP2PMsg(P2P_msg *msg) {
    free(msg);
}

void deinitP2PMsgGen(void *msg) {
    free((P2P_msg *)msg);
}

P2P_msg *p2pCopyMsg(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "initP2PMsg", "NULL msg");
        return NULL;
    }
    P2P_msg *new = initP2PMsg(msg->type);
    if (!new) {
        warnl("p2p-msg.c", "initP2PMsg", "fail initP2PMsg");
        return NULL;
    }
    new->nb_user_online = msg->nb_user_online;
    for (unsigned i = 0; i < msg->nb_user_online; i++) {
        strncpy(new->list_user_online[i], msg->list_user_online[i], SIZE_NAME);
    }
    strncpy(new->user_id, msg->user_id, SIZE_NAME);
    strncpy(new->private_ip, msg->private_ip, SIZE_IP_CHAR);
    strncpy(new->try_ip, msg->try_ip, SIZE_IP_CHAR);
    new->private_port = msg->private_port;
    new->public_port = msg->public_port;
    new->try_port = msg->try_port;
    return new;
}

P2P_msg_type p2pMsgGetType(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetType", "NULL msg");
        return UNDIFINED_P2P_TYPE;
    }
    return msg->type;
}

char *p2pMsgGetUserId(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetUserId", "NULL msg");
        return NULL;
    }
    char *id = malloc(SIZE_NAME);
    strncpy(id, msg->user_id, SIZE_NAME);
    return id;
}

GenList *p2pMsgGetListUserOnline(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetListUserOnline", "NULL msg");
        return NULL;
    }
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
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetPublicPort", "NULL msg");
        return -1;
    }
    return msg->public_port;
}

int p2pMsgGetPrivatePort(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetPrivatePort", "NULL msg");
        return -1;
    }
    return msg->private_port;
}

char *p2pMsgGetPrivateIp(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetPrivateIp", "NULL msg");
        return NULL;
    }
    char *ip = malloc(SIZE_IP_CHAR);
    strncpy(ip, msg->private_ip, SIZE_IP_CHAR);
    return ip;
}

char *p2pMsgGetTryIp(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetTryIp", "NULL msg");
        return NULL;
    }
    char *ip = malloc(SIZE_IP_CHAR);
    strncpy(ip, msg->try_ip, SIZE_IP_CHAR);
    return ip;
}

int p2pMsgGetTryPort(P2P_msg *msg) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgGetTryPort", "NULL msg");
        return -1;
    }
    return msg->try_port;
}

/*
    Setteur on P2P_msg*
*/
void p2pMsgSetType(P2P_msg *msg, P2P_msg_type type) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetType", "NULL msg");
        return;
    }
    msg->type = type;
}

void p2pMsgSetUserId(P2P_msg *msg, char *user_id) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetUserId", "NULL msg");
        return;
    }
    if (!user_id) {
        warnl("p2p-msg.c", "p2pMsgSetUserId", "NULL user_id");
        return;
    }
    strncpy(msg->user_id, user_id, SIZE_NAME);
}

void p2pMsgSetListUserOnline(P2P_msg *msg, GenList *list_online) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetListUserOnline", "NULL msg");
        return;
    }
    if (!list_online) {
        warnl("p2p-msg.c", "p2pMsgSetListUserOnline", "NULL list_online");
        return;
    }
    char *id;
    msg->nb_user_online = 0;
    for (unsigned i = 0; i < genListSize(list_online) && i < MAX_ONLINE; i++) {
        strncpy(msg->list_user_online[i], genListGet(list_online, i), SIZE_NAME);
        msg->nb_user_online++;
    }
}

void p2pMsgSetPublicPort(P2P_msg *msg, int port) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetPublicPort", "NULL msg");
        return;
    }
    msg->public_port = port;
}

void p2pMsgSetPrivatePort(P2P_msg *msg, int port) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetPrivatePort", "NULL msg");
        return;
    }
    msg->private_port = port;
}

void p2pMsgSetPrivateIp(P2P_msg *msg, char *ip) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetPrivateIp", "NULL msg");
        return;
    }
    if (!ip) {
        warnl("p2p-msg.c", "p2pMsgSetPrivateIp", "NULL ip");
        return;
    }
    strncpy(msg->private_ip, ip, SIZE_IP_CHAR);
}

void p2pMsgSetTryInfo(P2P_msg *msg, char *ip, int port) {
    if (!msg) {
        warnl("p2p-msg.c", "p2pMsgSetTryInfo", "NULL msg");
        return;
    }
    if (!ip) {
        warnl("p2p-msg.c", "p2pMsgSetTryInfo", "NULL ip");
        return;
    }
    strncpy(msg->try_ip, ip, SIZE_IP_CHAR);
    msg->try_port = port;
}

/*
    debug
*/

void p2pMsgPrintl(P2P_msg *msg) {
    printl("\n| type : <%d>", msg->type);
    printl("| user_id : <%s>", msg->user_id);
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