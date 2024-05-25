#include <client/tui.h>
#include <network/manager.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/command.h>
#include <types/message.h>
#include <types/p2p-msg.h>
#include <types/packet.h>
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
    return TUI_SUCCESS;
}

TUI_error stdinGetUserInput2(char *buffer, WINDOW *input_win) {
    char FUN_NAME[32] = "stdinGetUserInput2";
    wclear(input_win);
    wrefresh(input_win);
    mvwgetnstr(input_win, 1, 1, buffer, SIZE_INPUT - 1);
    return TUI_SUCCESS;
}

void *stdinHandler(void *arg) {
    char FUN_NAME[32] = "stdinHandler";
    Manager *manager = (Manager *)arg;
    char *buffer, *token;
    TUI_error tui_error;
    Command *command;
    Msg *msg;
    Packet *packet;
    CMD_error cmd_error;
    bool exited = false;
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_OPEN);
    while (!exited) {
        switch (tui_error = stdinGetUserInput(&buffer)) {
        case TUI_SUCCESS:
            if (*buffer == '/') {
                command = initCommand(buffer);
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
                switch (cmd_error) {
                case CMD_ERR_SUCCESS:
                    break;
                case CMD_ERR_WRONG_FUNCTION_CALL:
                    exitl(FILE_TUI, FUN_NAME, CMD_ERR_WRONG_FUNCTION_CALL, "malformed error shouldn't happen");
                case CMD_ERR_MISSING_ARG:
                    packet = initPacketTXT("RTFM : Wrong number of argument for this command\n" HELP_TXT);
                    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                    deinitPacket(&packet);
                    break;
                case CMD_ERR_INVALID_ARG:
                    packet = initPacketTXT("RTFM : Invalid argument (most likely user_id is malformed)" HELP_TXT);
                    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
                    deinitPacket(&packet);
                    break;
                }
                deinitCommand(&command);
            } else {
                // TODO: Chat To be Implemented
            }
        case TUI_INPUT_ERROR:
            exitl(FILE_TUI, FUN_NAME, TUI_INPUT_ERROR, "error when trying to parse input");
        case TUI_MEMORY_ALLOCATION_ERROR:
            exitl(FILE_TUI, FUN_NAME, TUI_MEMORY_ALLOCATION_ERROR, "most likely ran out of memory");
        case TUI_OUTPUT_FORMATTING_ERROR:
            exitl(FILE_TUI, FUN_NAME, tui_error, "Unreachable !!!");
        }
    }
    managerSetState(manager, MANAGER_MOD_INPUT, MANAGER_STATE_CLOSED);
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

TUI_error stdoutDisplayPacket2(Packet *packet, WINDOW *output_win) {
    char FUN_NAME[32] = "stdoutDisplayPacket2";
    char *output;

    switch (packet->type) {
    case PACKET_TXT:
        wprintw(output_win, "%s\n", packet->txt);
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
        wprintw(output_win, "%s\n", output);
        free(output);
        break;
    }
    wrefresh(output_win);

    return TUI_SUCCESS;
}

void *stdoutHandler(void *arg) {
    char FUN_NAME[32] = "stdoutHandler";
    Manager *manager = (Manager *)arg;
    bool exited = false;
    Packet *packet;
    char *buffer;
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_OPEN);
    while (!exited) {
        switch (managerReceiveBlocking(manager, MANAGER_MOD_OUTPUT, &packet)) {
        case MANAGER_ERR_SUCCESS:
            if (packet->type == PACKET_P2P_MSG && packet->p2p.type == P2P_CLOSE) {
                printf("Application closed\n");
                exited = true;
            } else if (stdoutDisplayPacket(packet) == TUI_OUTPUT_FORMATTING_ERROR) {
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
    }
    managerSetState(manager, MANAGER_MOD_OUTPUT, MANAGER_STATE_CLOSED);
    managerMainSendPthreadToJoin(manager, pthread_self());
    return NULL;
}

void initWindows(WINDOW *output_win, WINDOW *input_win) {
    initscr();
    cbreak();
    noecho();
    curs_set(TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    output_win = newwin(height - INPUT_HEIGHT, width, 0, 0);
    input_win = newwin(INPUT_HEIGHT, width, height - INPUT_HEIGHT, 0);

    scrollok(output_win, TRUE);
    keypad(input_win, TRUE);

    wrefresh(output_win);
    wrefresh(input_win);
}
