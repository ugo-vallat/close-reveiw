#include <stdlib.h>
#include <string.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_PACKET "packet.c"

Packet *initPacketTXT(char *txt) {
    char FUN_NAME[32] = "initPacketTXT";
    assertl(txt, FILE_PACKET, FUN_NAME, -1, "packet NULL");

    Packet *p = malloc(sizeof(Packet));
    p->type = PACKET_TXT;
    strncpy(p->txt, txt, SIZE_TXT);
    return p;
}

Packet *initPacketMsg(Msg *msg) {
    char FUN_NAME[32] = "initPacketMsg";
    assertl(msg, FILE_PACKET, FUN_NAME, -1, "msg NULL");

    Packet *p = malloc(sizeof(Packet));
    p->type = PACKET_MSG;
    msgCopy(&(p->msg), msg);
    return p;
}

Packet *initPacketP2PMsg(P2P_msg *msg) {
    char FUN_NAME[32] = "initPacketP2PMsg";
    assertl(msg, FILE_PACKET, FUN_NAME, -1, "msg NULL");

    Packet *p = malloc(sizeof(Packet));
    p->type = PACKET_P2P_MSG;
    p2pMsgCopy(&(p->p2p), msg);
    return p;
}

void deinitPacket(Packet **p) {
    char FUN_NAME[32] = "deinitPacket";
    assertl(p, FILE_PACKET, FUN_NAME, -1, "p NULL");
    assertl(*p, FILE_PACKET, FUN_NAME, -1, "*p NULL");

    free(*p);
    *p = NULL;
}

void deinitPacketGen(void *p) {
    char FUN_NAME[32] = "deinitPacketGen";
    assertl(p, FILE_PACKET, FUN_NAME, -1, "p NULL");

    free(p);
}

Packet *packetCopy(Packet *p) {
    char FUN_NAME[32] = "packetCopy";
    assertl(p, FILE_PACKET, FUN_NAME, -1, "p NULL");

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
