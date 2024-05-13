#include <network/packet.h>
#include <network/tls-com.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/logger.h>

int port;
char ip[CHAR_IP_SIZE];

int main(int argc, char *argv[]) {
    int ret;
    printf("[CLIENT] Starting...\n \t <+>--- OK\n");

    /* init client */
    init_logger(NULL);
    printf("[CLIENT] get args...\n");
    if (argc != 3) {
        printl("usage : %s <ip_server> <port_server>\n", argv[0]);
        return 1;
    }
    strncpy(ip, argv[1], CHAR_IP_SIZE);
    port = atoi(argv[2]);
    printf(" > IP : %s \n > Port : %d\n", ip, port);
    printf(" \t <+>--- OK\n");

    /* init tls com */
    printf("[CLIENT] initTLSInfos...\n");
    TLS_infos *infos = initTLSInfos(ip, port, CLIENT, NULL, NULL);
    printf(" \t <+>--- OK\n");

    printf("[CLIENT] openComTLS...\n");

    if (openComTLS(infos) != 0) {
        warnl("main", "main", "Echec connection");
        return 1;
    }
    printf(" \t <+>--- OK\n");

    /* communication */
    printf("[CLIENT] Send packet...\n");
    // sleep(1);
    Packet *p = malloc(sizeof(Packet));
    p->type = TXT;
    p->msg.size = 18;
    strncpy(p->msg.buffer, "Hello from client", SIZE_MSG_DATA);

    if (sendPacket(infos, p) != 0) {
        warnl("main", "main", "echec envoie");
        return 1;
    }
    printf(" \t <+>--- OK\n");

    printf("[CLIENT] Receive packet...\n");
    /* receive packet */
    while (!isPacketReceived(infos))
        ;
    ret = receivePacket(infos, &p);
    if (ret == TLS_SUCCESS) {
        printf("[CLIENT] Packet received : <%s>\n", p->msg.buffer);
        printf(" \t <+>--- OK\n");
        free(p);
    } else if (ret != TLS_CLOSE) {
        warnl("main.c", "main", "error received");
    }

    /* close com */
    GenList *l;
    printf("[CLIENT] closeComTLS...\n");
    fflush(stdout);
    ret = closeComTLS(infos, &l);
    if (ret != TLS_SUCCESS) {
        warnl("main.c", "main", "error closeComTLS");
    }

    while (!genListIsEmpty(l)) {
        printf("[CLIENT] Packet received : <%s>\n", ((Packet *)genListPop(l))->msg.buffer);
    }
    printf(" \t <+>--- OK\n");

    /* end communication */
    printf("[CLIENT] deleteTLSInfos...\n");
    deleteTLSInfos(&infos);
    printf(" \t <+>--- OK\n");

    close_logger();
    printf("[CLIENT] END\n");
    return 0;
}
