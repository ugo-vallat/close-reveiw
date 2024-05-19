
#include "types/message.h"
#include "utils/const-define.h"
#include <network/tls-com.h>
#include <stdio.h>
#include <types/packet.h>
#include <utils/logger.h>

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"

int port_server;
char ip_server[SIZE_IP_CHAR];
char user_id[SIZE_NAME];
TLS_infos *tls;

void getArgs(int argc, char *argv[]);
void loopChat();
void tryClient(char *act);
void okClient();
void koClient();

int main(int argc, char *argv[]) {
    bool end = false;
    char buff[SIZE_MSG_DATA];
    Packet *p;
    Msg *msg;
    TLS_error tls_error = TLS_RETRY;

    init_logger(NULL);
    tryClient("getArgs");
    getArgs(argc, argv);
    okClient();

    tryClient("initTLSInfos");
    tls = initTLSInfos(ip_server, port_server, TLS_CLIENT, NULL, NULL);
    okClient();

    tryClient("tlsOpenCom");
    tlsOpenCom(tls, NULL);
    okClient();

    tryClient("send");
    printf("\n > ");
    scanf("%s", buff);
    msg = initMsg(user_id, buff);
    printf(" >>> %s \n", msgToTXT(msg));
    p = initPacketMsg(msg);
    printf(" >>> %s \n", msgToTXT(&(p->msg)));
    deinitMsg(&msg);
    tlsSend(tls, p);
    deinitPacket(&p);
    okClient();

    tryClient("receive");
    while (tls_error == TLS_RETRY) {
        tls_error = tlsReceiveNonBlocking(tls, &p);
    }
    if (tls_error == TLS_SUCCESS) {
        printf("%s\n", msgToTXT(&(p->msg)));
        okClient();
    } else {
        koClient();
    }

    tryClient("close");
    tlsCloseCom(tls, NULL);
    okClient();

    close_logger();
    return 0;
}

void tryClient(char *act) {
    printf("\n %s[CLIENT] > %s %s\n", BLUE, act, RESET);
    fflush(stdout);
}

void okClient() {
    printf("\t\t %s <+>---------- OK %s\n", GREEN, RESET);
    fflush(stdout);
}

void koClient() {
    printf("\t\t %s <+>---------- KO %s\n", RED, RESET);
    fflush(stdout);
}

void displayUsage(char *bin) {
    printf("usage : %s <ip_server> <port_server> <id>\n", bin);
}

void getArgs(int argc, char *argv[]) {
    if (argc != 4) {
        displayUsage(argv[0]);
        exit(1);
    }
    strncpy(ip_server, argv[1], SIZE_IP_CHAR);
    port_server = atoi(argv[2]);
    if (port_server < 1024) {
        displayUsage(argv[0]);
        exit(1);
    }
    strncpy(user_id, argv[3], SIZE_NAME);
}
