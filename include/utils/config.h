#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <stdio.h>

#define CHAR_IP_SIZE 32
#define DIRECTORY_MAX_SIZE 128
#define BUFFER_SIZE 512

typedef struct {
    bool is_defined;
    char ip[CHAR_IP_SIZE];
    int port;
} t_addr_serv;

typedef struct {
    bool is_defined;

    char ip[CHAR_IP_SIZE];
    int local_port;
    int public_port;
} t_addr_user;

typedef struct {
    bool is_defined;

    bool user_consent;
    char path[BUFFER_SIZE];
} t_history;

typedef struct {
    bool is_defined;

    char certificate[BUFFER_SIZE];
    char key[BUFFER_SIZE];
} t_conf_ssl;

typedef struct {
    t_addr_user user;
    t_addr_serv server;
    t_history history;
    t_conf_ssl config_ssl;
} t_config;

bool getConfigFilePath(char config_file[DIRECTORY_MAX_SIZE]);

bool setConfig(FILE *config, t_config *configuration);

#endif // !__CONFIG_H__
