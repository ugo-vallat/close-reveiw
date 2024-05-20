#include <client/tui.h>
#include <network/manager.h>
#include <network/p2p-com.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <types/command.h>
#include <types/genericlist.h>
#include <types/packet.h>
#include <utils/logger.h>

#define FILE_COMMAND "command.c"

Type_cmd getCommandType(char *command) {
    char *cases[NB_COMMANDS] = {"list", "request", "accept", "reject", "close", "quit", "help"};
    for (int i = 0; i < NB_COMMANDS; i++) {
        if (strncmp(command, cases[i], strnlen(command, COMMAND_MAX_SIZE)) == 0) {
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
        deinitGenList(&(*command)->args, free);
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

CMD_error commandRequest(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandRequest";
    char *user_id;
    if (command->cmd != CMD_REQUEST) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 2) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }
    user_id = genListGet(command->args, 1);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid argument received");
        return CMD_ERR_INVALID_ARG;
    }
    p2pSendRequestConnection(manager, user_id);
    return CMD_ERR_SUCCESS;
}

CMD_error commandAnswer(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandAnswer";
    char *user_id;
    if (command->cmd != CMD_ACCEPT || command->cmd != CMD_REJECT) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    if (genListSize(command->args) != 2) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }
    user_id = genListGet(command->args, 1);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid argument received");
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
    if (genListSize(command->args) != 2) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid number of arguments given");
        return CMD_ERR_MISSING_ARG;
    }
    user_id = genListGet(command->args, 1);
    if (user_id == NULL || !isValidUserId(user_id)) {
        warnl(FILE_COMMAND, FUN_NAME, "invalid argument received");
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
    // TODO: To be Implemented
    return CMD_ERR_SUCCESS;
}

CMD_error commandHelp(Command *command, Manager *manager) {
    char FUN_NAME[32] = "commandHelp";
    if (command->cmd != CMD_HELP) {
        warnl(FILE_COMMAND, FUN_NAME, "called the wrong function");
        return CMD_ERR_WRONG_FUNCTION_CALL;
    }
    Packet *packet = initPacketTXT(HELP_TXT);
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
    Packet *packet = initPacketTXT(UNKNOWN_TXT);
    managerSend(manager, MANAGER_MOD_OUTPUT, packet);
    deinitPacket(&packet);
    return CMD_ERR_SUCCESS;
}
