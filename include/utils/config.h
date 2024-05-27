#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "utils/project_constants.h"
#include <stdbool.h>
#include <stdio.h>

#define CONFIG_CHAR_IP_SIZE SIZE_IP_CHAR
#define CONFIG_DIRECTORY_MAX_SIZE 128
#define CONFIG_BUFFER_SIZE 512

typedef struct {
    bool is_defined;
    char ip[CONFIG_CHAR_IP_SIZE];
    int port;
} t_addr_serv;

typedef struct {
    bool is_defined;

    char ip[CONFIG_CHAR_IP_SIZE];
    int local_port;
    int public_port;
} t_addr_user;

typedef struct {
    bool is_defined;

    bool user_consent;
    char path[CONFIG_BUFFER_SIZE];
} t_history;

typedef struct {
    bool is_defined;

    char certificate[CONFIG_BUFFER_SIZE];
    char key[CONFIG_BUFFER_SIZE];
} t_conf_ssl;

typedef struct {
    t_addr_user user;
    t_addr_serv server;
    t_history history;
    t_conf_ssl config_ssl;
} t_config;

/**
 * @brief DEPRECATED load config file path into buffer config_file
 *
 * @param config_file Buffer for file path
 * @return true if success, false otherwise
 */
bool getConfigFilePath(char config_file[CONFIG_DIRECTORY_MAX_SIZE]);

/**
 * @brief Load configuration into the t_config structure
 *
 * @return t_config*, NULL if error
 */
t_config *loadConfig(void);

/**
 * @brief delete the t_config structure and free memory
 *
 * @param config struct t_config
 */
void deinitConfig(t_config **config);

#endif // !__CONFIG_H__
