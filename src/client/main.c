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
    printf("[CLIENT] Starting...\n");

    init_logger(NULL);
    if (argc != 3) {
        printl("usage : %s <ip_server> <port_server>\n", argv[0]);
        return 1;
    }
    strncpy(ip, argv[1], CHAR_IP_SIZE);
    port = atoi(argv[2]);
    printf(" > IP : %s \n > Port : %d\n", ip, port);

    printf("[CLIENT] Create struct TLSInfos...\n");
    TLSInfos *infos = initTLSInfos(ip, port, CLIENT, NULL, NULL);

    printf("[CLIENT] Ready...\n");

    if (openComTLS(infos) != 0) {
        warnl("main", "main", "Echec connection");
        return 1;
    }
    printf("[CLIENT] Connected...\n");
    Packet *p = malloc(sizeof(Packet));
    p->type = TXT;
    p->msg.size = 18;
    strncpy(p->msg.buffer, "Hello from client", SIZE_MSG_DATA);
    if (sendPacket(infos, p) != 0) {
        warnl("main", "main", "echec envoie");
        return 1;
    }
    printf("[CLIENT] Packet sended\n");
    // sleep(1);
    deleteTLSInfos(&infos);
    printf("[CLIENT] TLSInfo deleted\n");
    close_logger();
    return 0;
}
