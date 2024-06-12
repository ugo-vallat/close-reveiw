#include "types/clientlist.h"
#include "types/genericlist.h"
#include <mariadb/mysql.h>
#include <server/cli.h>
#include <server/database-manager.h>
#include <stdio.h>
#include <utils/logger.h>
#include <signal.h>

#define FILE_NAME "cli"


typedef enum e_type_cmd_server {
    CMD_SRV_LIST = 0,
    CMD_SRV_CREATE = 1,
    CMD_SRV_DELETE = 2,
    CMD_SRV_QUIT = 3,
    CMD_SRV_HELP = 4,
    CMD_SRV_UNKNOWN = -1,
    CMD_SRV_ERROR = -2

} Type_cmd_server;

typedef struct e_command_server {
    Type_cmd_server type;
    GenList *args;
} Command_server;

Type_cmd_server getCommandTypeServer(char *command) {
    char *cases[NB_COMMANDS] = {"list", "create", "delete", "quit", "help"};
    for (int i = 0; i < NB_COMMANDS; i++) {
        if (strncmp(command, cases[i], strnlen(command, SIZE_MAX_CMD)) == 0) {
            return i;
        }
    }
    return CMD_SRV_UNKNOWN;
}

Command_server initCommandServer(char *buffer) {
    char FUN_NAME[32] = "initCommandServer";
    char *token;
    Command_server command;
    command.args = initGenList(sizeof(char *));
    printf("struct creer\n");
    if (command.args == NULL) {
        warnl(FILE_NAME, FUN_NAME, "fail initGenList args");
        command.type = CMD_SRV_ERROR;
        return command;
    }
    token = strtok(buffer, " ");
    command.type = getCommandTypeServer(token);

    while ((token = strtok(NULL, " ")) != NULL) {
        char *arg = strdup(token);
        if (arg == NULL) {
            warnl(FUN_NAME, FUN_NAME, "fail strdup arg");
            deinitGenList(&command.args, free);
            command.type = CMD_SRV_ERROR;
            return command;
        }
        genListAdd(command.args, arg);
    }
    free(buffer);
    return command;
}

void printListUser() {
    ClientList *l = getUserList(conn);
    printf("there are %d user:\n", clientListSize(l));
    for (unsigned i = 0; i < clientListSize(l); i++) {
        printf("user id = %d name = %s,", clientListGet(l, i)->id, clientListGet(l, i)->username);
    }
    printf("\n");
    deinitClientList(&l);
}

void help(){
    printf(SERVER_HELP_TXT);
}

void createUserCli(Command_server c) {
    if(genListSize(c.args) != 2){
        printf("wrong number of argument, use help for more information\n");
        return ;
    }
    char *name = genListGet(c.args, 0);
    printf(" nom utilisateur: %s\n", name);
    createUser(conn, name, genListGet(c.args, 1));
}

void deleteUserCli(Command_server c) {
    if(genListSize(c.args) != 1){
        printf("wrong number of argument, use help for more information\n");
        return ;
    }
    char *name = genListGet(c.args, 0);
    printf("utilisateur %s supprimer", name);
    deleteUser(conn, getId(conn, name));
}

void commandHandler(char c) {
}

void *cli(void *useless) {
    char *command;
    size_t size;
    int i;
    printf("debut cli\n");
    while (!end) {
        i = 0;
        getline(&command, &size, stdin);
        printf("ligne lu\n");
        while (command[i] != '\0') {
            if (command[i] == '\n') {
                command[i] = '\0';
            }
            i++;
        }
        printf("ligne sanitize\n");
        Command_server cmd = initCommandServer(command);
        printf("command creer\n");
        switch (cmd.type) {
        case CMD_SRV_CREATE:
            createUserCli(cmd);
            break;
        case CMD_SRV_DELETE:
            deleteUserCli(cmd);
            break;
        case CMD_SRV_LIST:
            printf("command list\n");
            printListUser();
            break;
        case CMD_SRV_HELP:
            printf("TODO"); // TODO
            break;
        case CMD_SRV_QUIT:
            end = true;
            break;
        case CMD_SRV_UNKNOWN:
            printf("unknown command use help for have the list of commande");
            warnl(FILE_NAME, "cli", "command unknown");
            break;
        case CMD_SRV_ERROR:
            break;
        }
        deinitGenList(&cmd.args, free);
        cmd.type = CMD_SRV_ERROR;
        command = NULL;
    }
    listAdd(thread, pthread_self());
    pthread_kill(nb_main, SIGUSR1);
    return NULL;
}
