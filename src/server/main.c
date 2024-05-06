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
    printf("[SERVER] Starting...\n");

    init_logger(NULL);
    if (argc != 3) {
        printl("usage : %s <ip_server> <port_server>\n", argv[0]);
        return 1;
    }
    port = atoi(argv[2]);
    strncpy(ip, argv[1], CHAR_IP_SIZE);
    printf(" > IP : %s \n > Port : %d\n", ip, port);

    printf("[SERVER] Create struct TLSInfos...\n");
    TLSInfos *infos = initTLSInfos(ip, port, SERVER, CERT_PATH, KEY_PATH);

    printf("[SERVER] Open tls com...\n");
    if (openComTLS(infos) != 0) {
        warnl("main", "main", "Echec connection");
        return 1;
    }
    printf("[SERVER] Connected...\n");
    Packet *p;
    if (receivePacket(infos, &p) != 0) {
        warnl("main", "main", "echec réception");
        return 1;
    }
    printf("[SERVER] Packet received : <%s>\n", p->msg.buffer);
    deleteTLSInfos(&infos);
    printf("[SERVER] TLSInfo deleted\n");
    close_logger();
    return 0;
}
