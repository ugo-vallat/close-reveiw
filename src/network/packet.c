#include <network/packet.h>
#include <stdlib.h>

void packetDelete(void *p) {
    free(p);
}