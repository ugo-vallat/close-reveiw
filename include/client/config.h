#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <stdio.h>

#define CHAR_IP_SIZE 32
#define DIRECTORY_MAX_SIZE 128
#define BUFFER_SIZE 512

typedef struct {
    char ip[CHAR_IP_SIZE];
    int port;
} t_addr;

typedef struct {
    bool user_consent;
    char path[BUFFER_SIZE];
} t_history;

typedef struct {
    char certificate[BUFFER_SIZE];
    char key[BUFFER_SIZE];
} t_conf_ssl;

bool getConfigFilePath(char config_file[DIRECTORY_MAX_SIZE]);

bool setConfig(FILE *config, t_addr *user, t_addr *server, t_history *history,
               t_conf_ssl *conf_ssl);

#endif // !__CONFIG_H__
