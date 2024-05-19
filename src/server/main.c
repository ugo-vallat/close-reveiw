#include <network/tls-com.h>
#include <pthread.h>
#include <stdio.h>
#include <types/packet.h>
#include <utils/logger.h>

#define CERT_PATH                                                                                  \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/"                  \
    "server-be-auto-cert.crt"
#define KEY_PATH                                                                                   \
    "/home/ugolinux/documents_linux/S6_Info_Linux/BE/close-review/config/server/server-be.key"

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
void tryServer(char *act);
void okServer();
void koServer();

int main(int argc, char *argv[]) {
    bool end = false;
    char buff[SIZE_MSG_DATA];
    Packet *p;
    Msg *msg;
    TLS_error tls_error = TLS_RETRY;

    init_logger(NULL);
    tryServer("getArgs");
    getArgs(argc, argv);
    okServer();

    tryServer("initTLSInfos");
    tls = initTLSInfos(ip_server, port_server, TLS_SERVER, CERT_PATH, KEY_PATH);
    okServer();

    tryServer("tlsOpenCom");
    tlsOpenCom(tls, NULL);
    okServer();

    tryServer("receive");
    while (tls_error == TLS_RETRY) {

        tls_error = tlsReceiveNonBlocking(tls, &p);
    }
    if (tls_error == TLS_SUCCESS) {
        printf("%s\n", msgToTXT(&(p->msg)));
        okServer();
    } else {
        koServer();
    }

    tryServer("send");
    printf("\n > ");
    scanf("%s", buff);
    msg = initMsg(user_id, buff);
    p = initPacketMsg(msg);
    deinitMsg(&msg);
    tlsSend(tls, p);
    deinitPacket(&p);
    okServer();

    tryServer("close");
    tlsCloseCom(tls, NULL);
    okServer();

    close_logger();
    return 0;
}

void tryServer(char *act) {
    printf("\n %s[CLIENT] > %s %s\n", BLUE, act, RESET);
    fflush(stdout);
}

void okServer() {
    printf("\t\t %s <+>---------- OK %s\n", GREEN, RESET);
    fflush(stdout);
}

void koServer() {
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
