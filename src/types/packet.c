#include <stdlib.h>
#include <string.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <utils/logger.h>
#include <utils/project_constants.h>


Packet *initPacketTXT(char *txt) {
        ASSERTL(txt,-1, "packet NULL")

    Packet *p = malloc(sizeof(Packet));
    p->type = PACKET_TXT;
    strncpy(p->txt, txt, SIZE_TXT);
    return p;
}

Packet *initPacketMsg(Msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    Packet *p = malloc(sizeof(Packet));
    p->type = PACKET_MSG;
    msgCopy(&(p->msg), msg);
    return p;
}

Packet *initPacketP2PMsg(P2P_msg *msg) {
        ASSERTL(msg,-1, "msg NULL")

    Packet *p = malloc(sizeof(Packet));
    memset(p, 0, sizeof(Packet));
    p->type = PACKET_P2P_MSG;
    p2pMsgCopy(&(p->p2p), msg);
    return p;
}

void deinitPacket(Packet **p) {
        ASSERTL(p,-1, "p NULL")
    ASSERTL(*p,-1, "*p NULL")

    free(*p);
    *p = NULL;
}

void deinitPacketGen(void *p) {
        ASSERTL(p,-1, "p NULL")

    free(p);
}

Packet *packetCopy(Packet *p) {
        ASSERTL(p,-1, "p NULL")

    Packet *new = malloc(sizeof(Packet));
    memcpy(new, p, sizeof(Packet));
    return new;
}

char *packetTypeToString(Packet_type type) {
    char *string = malloc(16);
    switch (type) {
    case PACKET_MSG:
        strncpy(string, "PACKET_MSG", 16);
        break;
    case PACKET_P2P_MSG:
        strncpy(string, "PACKET_P2P_MSG", 16);
        break;
    case PACKET_TXT:
        strncpy(string, "PACKET_TXT", 16);
        break;
    }
    return string;
}
