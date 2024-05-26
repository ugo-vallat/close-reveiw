#include "types/p2p-msg.h"
#include <arpa/inet.h>
#include <client/tui.h>
#include <netinet/in.h>
#include <network/manager.h>
#include <network/p2p-com.h>
#include <server/weak_password.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/command.h>
#include <types/genericlist.h>
#include <types/packet.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_COMMAND "command.c"

bool isValidCharacter(unsigned char character) {
    return character >= 'A' && character <= 'Z' || character >= 'a' && character <= 'z' ||
           character >= '0' && character <= '9' || character == '_';
}

bool isValidUserId(char *user_id) {
    for (int i = 0; user_id[i] == ' '; i++) {
        if (!isValidCharacter(user_id[i]))
            return false;
    }
    return true;
}

Type_cmd getCommandType(char *command) {
    char *cases[NB_COMMANDS] = {"list", "connect", "request", "direct", "accept", "reject", "close", "quit", "help"};
    for (int i = 0; i < NB_COMMANDS; i++) {
        if (strncmp(command, cases[i], strnlen(command, SIZE_MAX_CMD)) == 0) {
            return i;
        }
    }
    return CMD_UNKNOWN;
}

Command *initCommand(char *buffer) {
    char FUN_NAME[32] = "initCommand";
    char *token;
    Command *command = malloc(sizeof(Command));
    if (command == NULL) {
        warnl(FILE_COMMAND, FUN_NAME, "fail malloc Command");
        return NULL;
    }
    command->args = initGenList(sizeof(char *));
    if (command->args == NULL) {
        warnl(FILE_COMMAND, FUN_NAME, "fail initGenList args");
        free(command);
        return NULL;
    }
    token = strtok(buffer, " ");
    if (token && token[0] == '/') {
        command->cmd = getCommandType(token + 1);
    } else {
        warnl(FILE_COMMAND, FUN_NAME, "invalid command format");
        deinitCommand(&command);
        return NULL;
    }

    while ((token = strtok(NULL, " ")) != NULL) {
        char *arg = strdup(token);
        if (arg == NULL) {
            warnl(FILE_COMMAND, FUN_NAME, "fail strdup arg");
            deinitCommand(&command);
            return NULL;
        }
        genListAdd(command->args, arg);
    }
    free(buffer);
    return command;
}

void deinitCommand(Command **command) {
    if (*command != NULL) {
        deinitGenList(&((*command)->args), free);
        free(*command);
        *command = NULL;
    }
}

CMD_error commandList(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandList";
    if (command->cmd != CMD_LIST) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    p2pGetlistUsersAvailable(manager);
    return CMD_ERR_SUCCESS;
}

CMD_error commandConnect(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandList";
    char *user_id;
    char password[SIZE_HASH];
    if (command->cmd != CMD_CONNECT) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 2) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }

    user_id = genListGet(command->args, 0);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid user_id");
        return CMD_ERR_INVALID_ARG;
    }

    size_t password_size = strnlen(genListGet(command->args, 1), SIZE_PASSWORD);
    if (password_size >= SIZE_PASSWORD) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid password size");
        return CMD_ERR_INVALID_ARG;
    }
    password_to_md5_hash(genListGet(command->args, 1), password);
    p2pConnectToServer(manager, user_id, password);
    return CMD_ERR_SUCCESS;
}

CMD_error commandRequest(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandRequest";
    char *user_id;
    if (command->cmd != CMD_REQUEST) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 1) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }

    user_id = genListGet(command->args, 0);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid user_id");
        return CMD_ERR_INVALID_ARG;
    }
    p2pSendRequestConnection(manager, user_id);
    return CMD_ERR_SUCCESS;
}

CMD_error commandDirect(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandDirect";
    if (command->cmd != CMD_DIRECT) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 3) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }

    char *ip = genListGet(command->args, 1);
    struct sockaddr_in addr;
    if ((inet_pton(AF_INET, ip, &addr) != 1)) {
        warnl(FILE_COMMAND, FUN_NAME, "the given IP is invalid");
        return CMD_ERR_INVALID_ARG;
    }

    int port = 0;
    if (sscanf(genListGet(command->args, 2), "%d", &port) != 1 || port < 0 || port > 65536) {
        warnl(FILE_COMMAND, FUN_NAME, "the given port is invalid");
        return CMD_ERR_INVALID_ARG;
    }

    char *mode_string = genListGet(command->args, 0);
    bool is_client_mode = strncmp(mode_string, "-c", strlen("-c")) == 0;
    if (!is_client_mode && strncmp(mode_string, "-s", strlen("-s")) != 0) {
        warnl(FILE_COMMAND, FUN_NAME, "the given mode is invalid");
        return CMD_ERR_INVALID_ARG;
    }
    p2pStartDirectConnection(manager, is_client_mode ? TLS_CLIENT : TLS_SERVER, ip, port);
    return CMD_ERR_SUCCESS;
}

CMD_error commandAnswer(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandAnswer";
    char *user_id;
    if (command->cmd != CMD_ACCEPT && command->cmd != CMD_REJECT) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 1) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }
    user_id = genListGet(command->args, 0);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid user_id");
        return CMD_ERR_INVALID_ARG;
    }
    p2pRespondToRequest(manager, user_id, command->cmd == CMD_ACCEPT);
    return CMD_ERR_SUCCESS;
}

CMD_error commandClose(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandClose";
    char *user_id;
    if (command->cmd != CMD_CLOSE) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 1) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }
    user_id = genListGet(command->args, 0);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid user_id");
        return CMD_ERR_INVALID_ARG;
    }
    p2pCloseCom(manager, user_id);
    return CMD_ERR_SUCCESS;
}

CMD_error commandQuit(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandQuit";
    if (command->cmd != CMD_QUIT) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }

    /* create close request */
    char *sender = managerGetUser(manager);
    P2P_msg *p2p = initP2PMsg(P2P_CLOSE, sender);
    free(sender);
    Packet *packet = initPacketP2PMsg(p2p);

    /* send request to modules */
    managerSend(manager, MANAGER_MOD_SERVER, packet);
    managerSend(manager, MANAGER_MOD_PEER, packet);
    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    deinitPacket(&packet);
    deinitP2PMsg(&p2p);

    return CMD_ERR_SUCCESS;
}

CMD_error commandHelp(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandHelp";
    if (command->cmd != CMD_HELP && command->cmd != CMD_UNKNOWN) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    Packet *packet = initPacketTXT(HELP2_TXT);
    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    deinitPacket(&packet);
    return CMD_ERR_SUCCESS;
}

CMD_error commandUnknown(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandUnknown";
    if (command->cmd != CMD_UNKNOWN) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    Packet *packet = initPacketTXT(UNKNOWN2_TXT);
    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    deinitPacket(&packet);
    return CMD_ERR_SUCCESS;
}

char *commandTypeToChar(Type_cmd type) {
    switch (type) {
    case CMD_LIST:
        return "CMD_LIST";
    case CMD_CONNECT:
        return "CMD_CONNECT";
    case CMD_REQUEST:
        return "CMD_REQUEST";
    case CMD_DIRECT:
        return "CMD_DIRECT";
    case CMD_ACCEPT:
        return "CMD_ACCEPT";
    case CMD_REJECT:
        return "CMD_REJECT";
    case CMD_CLOSE:
        return "CMD_CLOSE";
    case CMD_QUIT:
        return "CMD_QUIT";
    case CMD_HELP:
        return "CMD_HELP";
    case CMD_UNKNOWN:
        return "CMD_UNKNOWN";
    }
}

char *commandErrorToChar(CMD_error error) {
    switch (error) {
    case CMD_ERR_SUCCESS:
        return "CMD_ERR_SUCCESS";
    case CMD_ERR_WRONG_FUNCTION_CALL:
        return "CMD_ERR_WRONG_FUNCTION_CALL";
    case CMD_ERR_MISSING_ARG:
        return "CMD_ERR_MISSING_ARG";
    case CMD_ERR_INVALID_ARG:
        return "CMD_ERR_INVALID_ARG";
    }
}
