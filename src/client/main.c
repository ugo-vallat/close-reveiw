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
    port = atoi(argv[1]);
    strncpy(ip, argv[2], CHAR_IP_SIZE);

    printf("[CLIENT] Create struct TLSInfos...\n");
    TLSInfos *infos = initTLSInfos(ip, port, CLIENT, NULL, NULL);

    printf("[CLIENT] Ready ! Go ? (Y/n)\n");
    char buff[256];
    fscanf(stdout, "%255s", buff);
    if (strcmp(buff, "") != 0 && strcmp(buff, "y") != 0 && strcmp(buff, "Y") != 0) {
        printf("[CLIENT] Stopping connection\n");
        deleteTLSInfos(&infos);
        return 0;
    }
    if (openComTLS(infos) != 0) {
        warnl("main", "main", "Echec connection");
    }
    printf("[CLIENT] Connected...\n");
    sleep(1);
    Packet p;
    p.type = TXT;
    p.msg.size = 18;
    strncpy(p.msg.buffer, "Hello from client", SIZE_MSG_DATA);
    if (sendPacket(infos, &p) != 0) {
        warnl("main", "main", "echec envoie");
    }
    printf("[CLIENT] Packet sended\n");
    deleteTLSInfos(&infos);
    printf("[CLIENT] TLSInfo deleted\n");
    close_logger();
    return 0;
}
