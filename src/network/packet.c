#include <network/packet.h>
#include <stdlib.h>

void packetDelete(void *p) {
    free(p);
}

Packet *packetCopy(Packet *p) {
    Packet *new = malloc(sizeof(Packet));
    new->msg = p->msg;
    new->type = p->type;
    return new;
}