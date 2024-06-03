#include <client/tui.h>
#include <curses.h>
#include <ncurses.h>
#include <network/chat.h>
#include <network/manager.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/command.h>
#include <types/genericlist.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
#include <unistd.h>
#include <utils/logger.h>

#define FILE_TUI "tui.c"

TUI_error stdinGetUserInput(char **buffer) {
    char FUN_NAME[32] = "stdinGetUserInput";
    *buffer = calloc(SIZE_INPUT, sizeof(char));
    if (buffer == NULL) {
        warnl(FILE_TUI, FUN_NAME, "fail calloc buffer char[SIZE_DATA_PACKET]");
        return TUI_MEMORY_ALLOCATION_ERROR;
    }
    size_t size_allocated = SIZE_INPUT;
    if (getline(buffer, &size_allocated, stdin) == -1) {
        warnl(FILE_TUI, FUN_NAME, "fail getline buffer");
        free(buffer);
        return TUI_INPUT_ERROR;
    }
    int i = 0;
    while ((*buffer)[i] != '\0') {
        if ((*buffer)[i] == '\n') {
            (*buffer)[i] = '\0';
        }
        i++;
    }
    return TUI_SUCCESS;
}

TUI_error stdinGetUserInput2(char **buffer, WINDOW *input_win) {
    char FUN_NAME[32] = "stdinGetUserInput2";
    *buffer = calloc(SIZE_INPUT, sizeof(char));
    if (buffer == NULL) {
        warnl(FILE_TUI, FUN_NAME, "fail calloc buffer char[SIZE_DATA_PACKET]");
        return TUI_MEMORY_ALLOCATION_ERROR;
    }
    wclear(input_win);
    mvwgetnstr(input_win, 1, 1, *buffer, SIZE_INPUT - 1);
    int i = 0;
    while ((*buffer)[i] != '\0') {
        if ((*buffer)[i] == '\n') {
            (*buffer)[i] = '\0';
        }
        i++;
    }
    wrefresh(input_win);
    return TUI_SUCCESS;
}

void *stdinHandler(void *arg) {
    char FUN_NAME[32] = "stdinHandler";
    Manager *manager = (Manager *)arg;
    int height, width;
    WINDOW *input_win;
    char *buffer, *token;
    TUI_error tui_error;
    Command *command;
    Msg *msg;
    Packet *packet;
    CMD_error cmd_error;
    bool exited = false;

    // Ncurses Initialization
    getmaxyx(stdscr, height, width);
    input_win = newwin(INPUT_HEIGHT, width, height - INPUT_HEIGHT, 0);
    keypad(input_win, TRUE);
    echo();
    wrefresh(input_win);

    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_OPEN);
    while (!exited) {
        switch (tui_error = stdinGetUserInput2(&buffer, input_win)) {
        case TUI_SUCCESS:
            if (*buffer == '/') {
                command = initCommand(buffer);
                if (command == NULL) {
                    cmd_error = CMD_ERR_INVALID_ARG;
                } else {
                    switch (command->cmd) {
                    case CMD_LIST:
                        cmd_error = commandList(command, manager);
                        break;
                    case CMD_CONNECT:
                        cmd_error = commandConnect(command, manager);
                        break;
                    case CMD_REQUEST:
                        cmd_error = commandRequest(command, manager);
                        break;
                    case CMD_DIRECT:
                        cmd_error = commandDirect(command, manager);
                        break;
                    case CMD_ACCEPT:
                    case CMD_REJECT:
                        cmd_error = commandAnswer(command, manager);
                        break;
                    case CMD_CLOSE:
                        cmd_error = commandClose(command, manager);
                        break;
                    case CMD_QUIT:
                        cmd_error = commandQuit(command, manager);
                        exited = true;
                        break;
                    case CMD_UNKNOWN:
                        cmd_error = commandUnknown(command, manager);
                    case CMD_HELP:
                        cmd_error = commandHelp(command, manager);
                        break;
                    }
                }
                switch (cmd_error) {
                case CMD_ERR_SUCCESS:
                    break;
                case CMD_ERR_WRONG_FUNCTION_CALL:
                    warnl(FILE_TUI, FUN_NAME, "malformed error shouldn't happen <%s>", commandTypeToChar(command->cmd));
                    deinitPacket(&packet);
                    break;
                case CMD_ERR_MISSING_ARG:
                    packet = initPacketTXT("RTFM : Wrong number of argument for this command\n" HELP2_TXT);
                    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                    deinitPacket(&packet);
                    break;
                case CMD_ERR_INVALID_ARG:
                    packet = initPacketTXT("RTFM : Invalid argument (most likely user_id is malformed)" HELP2_TXT);
                    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                    deinitPacket(&packet);
                    break;
                }
                deinitCommand(&command);
            } else {
                chatSendMessage(manager, buffer);
            }
            break;
        case TUI_INPUT_ERROR:
            exitl(FILE_TUI, FUN_NAME, TUI_INPUT_ERROR, "error when trying to parse input");
        case TUI_MEMORY_ALLOCATION_ERROR:
            exitl(FILE_TUI, FUN_NAME, TUI_MEMORY_ALLOCATION_ERROR, "most likely ran out of memory");
        case TUI_OUTPUT_FORMATTING_ERROR:
            exitl(FILE_TUI, FUN_NAME, tui_error, "Unreachable !!!");
        }
    }
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_CLOSED);
    delwin(input_win);
    managerMainSendPthreadToJoin(manager, pthread_self());
    return NULL;
}

TUI_error stdoutDisplayPacket(Packet *packet) {
    char FUN_NAME[32] = "stdoutDisplayPacket";
    char *output;
    switch (packet->type) {
    case PACKET_TXT:
        printf("%s\n", packet->txt);
        break;
    case PACKET_MSG:
        if ((output = msgToTXT(&packet->msg)) == NULL) {
            warnl(FILE_TUI, FUN_NAME, "failed to format Msg to TXT");
            return TUI_OUTPUT_FORMATTING_ERROR;
        }
        printf("%s\n", output);
        free(output);
        break;
    case PACKET_P2P_MSG:
        if ((output = p2pMsgToTXT(&packet->p2p)) == NULL) {
            warnl(FILE_TUI, FUN_NAME, "failed to format p2pMsg to TXT");
            return TUI_OUTPUT_FORMATTING_ERROR;
        }
        printf("%s\n", output);
        free(output);
        break;
    }
    return TUI_SUCCESS;
}

TUI_error stdoutDisplayPacket2(Packet *packet, WINDOW *output_win, Manager *manager) {
    char FUN_NAME[32] = "stdoutDisplayPacket2";
    char *output, *peer_id;

    switch (packet->type) {
    case PACKET_TXT:
        wprintw(output_win, "%s\n\n", packet->txt);
        break;
    case PACKET_MSG:
        if ((output = msgToTXT(&packet->msg)) == NULL) {
            warnl(FILE_TUI, FUN_NAME, "failed to format Msg to TXT");
            return TUI_OUTPUT_FORMATTING_ERROR;
        }
        wprintw(output_win, "%s\n", output);
        free(output);
        break;
    case PACKET_P2P_MSG:
        if ((output = p2pMsgToTXT(&packet->p2p)) == NULL) {
            warnl(FILE_TUI, FUN_NAME, "failed to format p2pMsg to TXT");
            return TUI_OUTPUT_FORMATTING_ERROR;
        }
        wprintw(output_win, "%s", output);
        P2P_msg_type type = p2pMsgGetType(&packet->p2p);
        switch (type) {
        case P2P_CONNECTION_OK:
            wprintw(output_win, "\t\t=== Welcome %s ===\n\n", managerGetUser(manager));
            break;
        case P2P_CON_SUCCESS:
            peer_id = p2pMsgGetPeerId(&packet->p2p);
            wclear(output_win);
            wprintw(output_win, "\t\t=== Communication with %s started : ===\n", peer_id);
            free(peer_id);
            break;
        case P2P_CONNECTION_SERVER:
        case P2P_CONNECTION_KO:
        case P2P_ACCEPT:
        case P2P_REJECT:
        case P2P_REQUEST_IN:
        case P2P_REQUEST_OUT:
        case P2P_GET_AVAILABLE:
        case P2P_AVAILABLE:
        case P2P_GET_INFOS:
        case P2P_INFOS:
        case P2P_CON_FAILURE:
        case P2P_TRY_SERVER_MODE:
        case P2P_TRY_CLIENT_MODE:
        case P2P_CLOSE:
            break;
        }
        free(output);
        break;
    }
    usleep(32000);
    wrefresh(output_win);
    return TUI_SUCCESS;
}

void *stdoutHandler(void *arg) {
    char FUN_NAME[32] = "stdoutHandler";
    Manager *manager = (Manager *)arg;
    int height, width;
    WINDOW *border_win, *output_win;
    bool exited = false;
    Packet *packet;
    char *buffer;

    // Ncurses Initialization
    getmaxyx(stdscr, height, width);
    border_win = newwin(height - INPUT_HEIGHT, width, 0, 0);
    output_win = derwin(border_win, height - INPUT_HEIGHT - 2, width - 2, 1, 1);
    scrollok(output_win, TRUE);
    box(border_win, 0, 0);
    wclear(output_win);
    usleep(32000);
    wrefresh(border_win);
    wrefresh(output_win);

    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_OPEN);
    while (!exited) {
        switch (managerReceiveBlocking(manager, MANAGER_MOD_OUTPUT, &packet)) {
        case MANAGER_ERR_SUCCESS:
            if (packet->type == PACKET_P2P_MSG && packet->p2p.type == P2P_CLOSE) {
                printf("Application closed\n");
                exited = true;
            } else if (stdoutDisplayPacket2(packet, output_win, manager) == TUI_OUTPUT_FORMATTING_ERROR) {
                buffer = packetTypeToString(packet->type);
                printf("Couldn't display the recieved packet of type : %s\n", buffer);
                free(buffer);
            }
            break;
        case MANAGER_ERR_ERROR:
            warnl(FILE_TUI, FUN_NAME, "manager has encountered an error");
            break;
        case MANAGER_ERR_RETRY:
            break;
        case MANAGER_ERR_CLOSED:
            warnl(FILE_TUI, FUN_NAME, "manager was closed unexpectedly");
            exited = true;
            break;
        }
        deinitPacket(&packet);
        usleep(32000);
        wrefresh(border_win);
    }
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_CLOSED);
    delwin(output_win);
    delwin(border_win);
    managerMainSendPthreadToJoin(manager, pthread_self());
    return NULL;
}
