#include "utils/genericlist.h"
#include <network/packet.h>
#include <network/tls-com.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/logger.h>

#define CERT_PATH                                                                                  \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define KEY_PATH                                                                                   \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

int port;
char ip[CHAR_IP_SIZE];

int main(int argc, char *argv[]) {
    int ret;
    printf("[SERVER] Starting...\n \t <+>--- OK\n");

    init_logger(NULL);
    printf("[SERVER] get args...\n");
    if (argc != 3) {
        printl("usage : %s <ip_server> <port_server>\n", argv[0]);
        return 1;
    }
    port = atoi(argv[2]);
    strncpy(ip, argv[1], CHAR_IP_SIZE);
    printf(" > IP : %s \n > Port : %d\n", ip, port);
    printf(" \t <+>--- OK\n");

    printf("[SERVER] initTLSInfos...\n");
    TLS_infos *infos = initTLSInfos(ip, port, SERVER, CERT_PATH, KEY_PATH);
    printf(" \t <+>--- OK\n");

    printf("[SERVER] openComTLS...\n");
    if (openComTLS(infos) != 0) {
        warnl("main", "main", "Echec connection");
        return 1;
    }
    printf(" \t <+>--- OK\n");

    printf("[SERVER] Receive packet...\n");
    Packet *p;
    /* receive packet */
    while (!isPacketReceived(infos))
        ;
    ret = receivePacket(infos, &p);
    if (ret == TLS_SUCCESS) {
        printf("[SERVER] Packet received : <%s>\n", p->msg.buffer);
        printf(" \t <+>--- OK\n");
        free(p);
    } else if (ret != TLS_CLOSE) {
        warnl("main.c", "main", "error received");
    }

    /* send packet */
    printf("[SERVER] Send packet...\n");
    p = malloc(sizeof(Packet));
    p->type = TXT;
    p->msg.size = 18;
    strncpy(p->msg.buffer, "Hello from server", SIZE_MSG_DATA);

    if (sendPacket(infos, p) != 0) {
        warnl("main", "main", "echec envoie");
        return 1;
    }
    printf(" \t <+>--- OK\n");

    GenList *l;
    printf("[SERVER] closeComTLS...\n");
    fflush(stdout);
    ret = closeComTLS(infos, &l);
    if (ret != TLS_SUCCESS) {
        warnl("main.c", "main", "error closeComTLS");
    }

    while (!genListIsEmpty(l)) {
        printf("[SERVER] Packet received : <%s>\n", ((Packet *)genListPop(l))->msg.buffer);
    }
    printf(" \t <+>--- OK\n");

    /* delete */
    printf("[SERVER] deleteTLSInfos...\n");
    deleteTLSInfos(&infos);
    printf(" \t <+>--- OK\n");
    close_logger();
    printf("[SERVER] END\n");
    return 0;
}
