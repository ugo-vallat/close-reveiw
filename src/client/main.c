#include "network/p2p-com.h"
#include "utils/genericlist.h"
#include "utils/message.h"
#include <network/gestionary-com.h>
#include <network/packet.h>
#include <network/tls-com.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/logger.h>

#define BLUE "\033[38;5;45m"
#define GREEN "\033[38;5;82m"
#define RED "\033[38;5;196m"
#define RESET "\033[0m"
#define SIZE_CMD 64

typedef enum e_cmd_tyme {
    C_UNKOWN,
    C_CONNECTED,
    C_CONNECT,
    C_ACCEPT,
    C_REJECT,
    C_REQUEST_IN,
    C_CLOSE,
    C_QUIT,
} Cmd_type;

typedef struct s_cmd {
    Cmd_type type;
    char id[SIZE_NAME];
} Cmd;

int port_server;
char ip_server[SIZE_IP_CHAR];
char user_id[SIZE_NAME];
P2P_infos *p2p;
Gestionary_com *gest_server;
Gestionary_com *gest_peer;

void getArgs(int argc, char *argv[]);
void connectToServer();
Cmd *getCmd(char *input);
int displayUserConnected();
int displayRequestIn();
int requestConnectionPeer(char *id_peer);
int respondRequestConnection(char *id_peer, bool rep);
void closeComPeer();
void quit();
void loopCmd();
void loopChat();
void tryClient(char *act);
void okClient();

int main(int argc, char *argv[]) {

    init_logger(NULL);

    /* get args */
    tryClient("get args");
    getArgs(argc, argv);
    printf("Hello <%s>\n", user_id);
    okClient();

    /* connection to server */
    tryClient("connect server");
    connectToServer();
    if (!gest_server) {
        warnl("main", "main", "fail create gest_server");
        return 1;
    }
    okClient();
    /* init P2P */
    tryClient("createP2P_infos");
    p2p = createP2P_infos(gest_server);
    if (!p2p) {
        warnl("main", "main", "fail create p2p");
        return 1;
    }
    okClient();

    /* main loop */
    while (1) {
        loopCmd();
        loopChat();
    }

    close_logger();
    return 0;
}

void loopCmd() {
    int ret;
    bool end = false;
    Cmd *cmd;
    char *input;
    size_t size;
    while (!end) {
        printf(" > ");
        fgets(input, SIZE_CMD, stdin);
        cmd = getCmd(input);
        if (!cmd)
            continue;
        switch (cmd->type) {
        case C_CONNECTED:
            displayUserConnected();
            break;
        case C_REQUEST_IN:
            displayRequestIn();
            break;
        case C_ACCEPT:
            if (respondRequestConnection(cmd->id, true) == 1) {
                printf(" %s> CONNECTED%s\n", GREEN, RESET);
                end = true;
            } else {
                printf(" %s> NOT CONNECTED%s\n", RED, RESET);
            }
            break;
        case C_REJECT:
            respondRequestConnection(cmd->id, false);
            break;
        case C_QUIT:
            quit();
            break;
        case C_CONNECT:
            if (requestConnectionPeer(cmd->id) == 1) {
                printf(" %s> CONNECTED%s\n", GREEN, RESET);
                end = true;
            } else {
                printf(" %s> NOT CONNECTED%s\n", RED, RESET);
            }
            break;
        default:
            warnl("main.c", "main", "error type cmd");
            break;
        }
    }
}

void *getMessagesFromUser(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    char *line;
    GenList *buff = (GenList *)arg;
    while (1) {
        pthread_testcancel();
        line = malloc(SIZE_MSG_DATA);
        fgets(line, SIZE_MSG_DATA, stdin);
        genListAdd(buff, line);
    }
    return NULL;
}

void loopChat() {
    bool end = false;
    unsigned long nbt;
    char *data;
    int ret;
    Cmd *cmd;
    Msg *msg;
    tryClient(" ICI ");
    GenList *buff = createGenList(128);

    /* create thread read */
    pthread_create(&nbt, NULL, getMessagesFromUser, (void *)buff);
    while (!end) {
        tryClient("1");
        /* input user */
        if (!genListIsEmpty(buff)) {
            tryClient("2");
            data = genListRemove(buff, 0);
            tryClient("3");
            /* cmd */
            if (data[0] == '/') {
                tryClient("4");
                cmd = getCmd(data);
                if (cmd->type == C_CLOSE || cmd->type == C_QUIT) {
                    end = true;
                } else {
                    warnl("main.c", "loopChat", "invalid command");
                }
                if (cmd)
                    free(cmd);
                free(data);
            } else {
                tryClient("5");
                msg = createMsg(user_id, data);
                ret = gestionarySendMsg(gest_peer, msg);
                if (ret == 1) {
                    printf("\r\n");
                    printMsg(msg);
                    printf("\n > ");
                    deleteMsg(msg);
                } else if (ret == 0) {
                    end = !gestionaryIsComOpen(gest_peer);
                } else {
                    warnl("main.c", "loopChat", "fail receive msg");
                }
            }
        }
        tryClient("6");
        ret = gestionaryReceiveMsg(gest_peer, &msg);
        tryClient("7");
        if (ret == 1)
            printMsg(msg);
    }

    pthread_join(nbt, NULL);
    deleteGenList(&buff, free);
    if (cmd->type == C_CLOSE)
        closeComPeer();
    if (cmd->type == C_QUIT)
        quit();
}

void tryClient(char *act) {
    printf("\n %s[CLIENT] > %s %s\n", BLUE, act, RESET);
    fflush(stdout);
}

void okClient() {
    printf("\t\t %s <+>---------- OK %s\n", GREEN, RESET);
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

void connectToServer() {
    int ret;
    TLS_infos *tls;

    /* init tls com */
    tls = initTLSInfos(ip_server, port_server, CLIENT, NULL, NULL);

    if (openComTLS(tls, NULL) != 0) {
        warnl("main", "main", "Echec connection");
        exit(1);
    }
    Packet *p;
    gest_server = createGestionaryCom(tls);
    if (!gest_server) {
        warnl("main.c", "main", "error creating gestionary server");
        exit(1);
    }
    gestionarySendTxt(gest_server, user_id);
}

void displayHelp() {
    printf("usage : /[cmd] [agrv]\n\n");
    printf("\t/list\t\tdisplay list connected users\n");
    printf("\t/request\t\tdisplay user requesting connection\n");
    printf("\t/connect [user]\t\tsend request connection to user\n");
    printf("\t/accept\t\taccept request from user\n");
    printf("\t/reject\t\treject request from user\n");
    printf("\t/close\t\tclose connection with peer\n");
    printf("\t/quit\t\tclose app\n");
}

GenList *splitCmd(char *data) {
    char *string;
    bool end = false;
    GenList *list = createGenList(8);
    int i = 0, j = 0;
    string = malloc(SIZE_DATA_PACKET);
    while (!end && i < SIZE_DATA_PACKET) {
        if (data[i] == ' ') {
            string[j++] = '\0';
            genListAdd(list, string);
            string = malloc(SIZE_DATA_PACKET);
            j = 0;
        } else if (data[i] == '\0' || data[i] == '\n') {
            string[j++] = '\0';
            genListAdd(list, string);
            end = true;
        } else {
            string[j] = data[i];
            j++;
        }
        i++;
    }
    return list;
}

Cmd_type getCmdType(char *cmd) {
    if (strcmp(cmd, "/list") == 0)
        return C_CONNECTED;
    if (strcmp(cmd, "/request") == 0)
        return C_REQUEST_IN;
    if (strcmp(cmd, "/connect") == 0)
        return C_CONNECT;
    if (strcmp(cmd, "/accept") == 0)
        return C_ACCEPT;
    if (strcmp(cmd, "/reject") == 0)
        return C_REJECT;
    if (strcmp(cmd, "/close") == 0)
        return C_CLOSE;
    if (strcmp(cmd, "/quit") == 0)
        return C_QUIT;
    return C_UNKOWN;
}

Cmd *getCmd(char *input) {
    GenList *args = splitCmd(input);
    if (genListSize(args) > 2) {
        warnl("main.c", "main", "invalide cmd <%s> (size > 2)", input);
        displayHelp();
        return NULL;
    }
    Cmd *cmd = malloc(sizeof(Cmd));
    memset(cmd, 0, sizeof(Cmd));
    cmd->type = getCmdType(genListGet(args, 0));
    switch (cmd->type) {
    case C_CONNECTED:
    case C_REQUEST_IN:
    case C_ACCEPT:
    case C_REJECT:
    case C_CLOSE:
    case C_QUIT:
        if (genListSize(args) != 1) {
            warnl("main.c", "main", "invalide cmd <%s> (invalid number arguments)", input);
            displayHelp();
            return NULL;
        }
        return cmd;
        break;
    case C_CONNECT:
        if (genListSize(args) != 2) {
            warnl("main.c", "main", "invalide cmd <%s> (invalid number arguments)", input);
            displayHelp();
            return NULL;
        }
        strncpy(cmd->id, genListGet(args, 1), SIZE_NAME);
        return cmd;
        break;
    case C_UNKOWN:
        if (genListSize(args) != 2) {
            warnl("main.c", "main", "invalide cmd <%s> (unknown)", input);
            displayHelp();
            return NULL;
        }
        free(cmd);
        return NULL;
    }
}

char *userIdToChar(void *id) {
    char *id_char = malloc(SIZE_NAME);
    strncpy(id_char, (char *)id, SIZE_NAME);
    return id_char;
}

int displayUserConnected() {
    GenList *list;
    list = p2pGetListUserConnected(p2p);
    if (!list) {
        warnl("main.c", "displayUserConnected", "error getListUserConnected");
        return -1;
    }
    for (unsigned i = 0; i < genListSize(list); i++) {
        printf(" | %s\n", (char *)genListGet(list, i));
    }
    return 0;
}

int displayRequestIn() {
    char *id = p2pGetRequestConnection(p2p);
    if (!id) {
        warnl("main.c", "displayRequestIn", "erro getRequestP2PConnection");
        return -1;
    }
    printf(" REQUEST IN : %s\n", id);
    return 0;
}
int requestConnectionPeer(char *id_peer) {
    return p2pSendRequestConnection(p2p, id_peer, &gest_peer);
}

int respondRequestConnection(char *id_peer, bool rep) {
    return p2pAcceptConnection(p2p, id_peer, rep, &gest_peer);
}

void closeComPeer() {
    deleteGestionaryCom(&gest_peer);
    printf("\n\t <+>----- END COM -----<+> \n\n");
}
void quit() {
    deleteGestionaryCom(&gest_peer);
    deleteGestionaryCom(&gest_server);
    printf("\n\t <+>----- END -----<+> \n\n");
    exit(0);
}

// int main(int argc, char *argv[]) {
//     int ret;
//     TLS_infos *tls;
//     Gestionary_com *gest;

//     printf("[CLIENT] Starting...\n \t <+>--- OK\n");

//     /* init client */
//     init_logger(NULL);
//     printf("[CLIENT] get args...\n");
//     if (argc != 3) {
//         printl("usage : %s <ip_server> <port_server>\n", argv[0]);
//         return 1;
//     }
//     strncpy(ip, argv[1], SIZE_IP_CHAR);
//     port = atoi(argv[2]);
//     printf(" > IP : %s \n > Port : %d\n", ip, port);
//     printf(" \t <+>--- OK\n");

//     /* init tls com */
//     printf("[CLIENT] initTLSInfos...\n");
//     tls = initTLSInfos(ip, port, CLIENT, NULL, NULL);
//     printf(" \t <+>--- OK\n");

//     printf("[CLIENT] openComTLS...\n");

//     if (openComTLS(tls) != 0) {
//         warnl("main", "main", "Echec connection");
//         return 1;
//     }
//     printf(" \t <+>--- OK\n");

//     printf("[CLIENT] createGestionaryCom...\n");
//     gest = createGestionaryCom(tls);
//     printf(" \t <+>--- OK\n");

//     /* communication */
//     printf("[CLIENT] Send packet...\n");
//     // sleep(1);
//     Msg *msg = malloc(sizeof(Msg));
//     memset(msg, 0, sizeof(Msg));
//     strncpy(msg->buffer, "hello from client !", SIZE_MSG_DATA);
//     strncpy(msg->sender, "CLIENT", SIZE_NAME);
//     gestionarySendMsg(gest, msg);
//     printf(" \t <+>--- OK\n");

//     /* receive packet */
//     printf("[CLIENT] Receive packet...\n");
//     while ((ret = gestionaryReceiveMsg(gest, &msg)) < 1) {
//         if (ret == -1 && gestionaryIsComOpen(gest) == false) {
//             warnl("main.c", "main", "error com closed");
//             return 1;
//         }
//     }
//     printf("[CLIENT] message from <%s> : <%s>\n", msg->sender, msg->buffer);
//     printf(" \t <+>--- OK\n");

//     /* close com */
//     printf("[CLIENT] deleteGestionaryCom...\n");
//     deleteGestionaryCom(&gest);
//     printf(" \t <+>--- OK\n");
//     close_logger();
//     printf("[CLIENT] END\n");
//     return 0;
// }
