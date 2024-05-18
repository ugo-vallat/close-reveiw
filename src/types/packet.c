#include "utils/logger.h"
#include <network/packet.h>
#include <stdlib.h>
#include <string.h>

Packet *initPacket(Packet_type type, void *buff, unsigned long size) {
    if (!buff) {
        warnl("packet.c", "cretaePacket", "null buffer");
        return NULL;
    }
    if (size >= SIZE_DATA_PACKET) {
        warnl("packet.c", "cretaePacket", "to much data (%ld > %ld)", size, SIZE_DATA_PACKET);
        return NULL;
    }
    Packet *p = malloc(sizeof(Packet));
    if (!p) {
        warnl("packet.c", "cretaePacket", "fail malloc packet");
        return NULL;
    }
    memset(p, 0, sizeof(Packet));
    p->type = type;
    p->size = size;
    memcpy(p->data, buff, size);
    return p;
}

void deinitPacket(Packet *p) {
    free(p);
}

void deinitPacketGen(void *p) {
    free((Packet *)p);
}

Packet *packetCopy(Packet *p) {
    return initPacket(p->type, p->data, p->size);
}