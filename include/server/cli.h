#ifndef __CLI_H__
#define __CLI_H__

#include "types/clientlist.h"
#include "types/list.h"
#include <mariadb/mysql.h>

#define SERVER_HELP_TXT                                                                                                \
    "Available Commands:\n"                                                                                            \
    "\n"                                                                                                               \
    "list:\t\tDisplays a list of all users.\n"                                                                         \
    "delete <username>:\tDelete the user\n"                                                                            \
    "create <username> <password>:\t\tCreate the user\n"                                                               \
    "quit:\t\tExits the "                                                                                              \
    "application.\n"                                                                                                   \
    "help:\t\tShows this help "                                                                                        \
    "message."

extern MYSQL *conn;
extern ClientList *user;
extern List *thread;
extern bool end;

void *cli(void *arg);

#endif
