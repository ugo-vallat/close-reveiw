#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>
#include <utils/project_constants.h>

#define CONFIG_CHAR_IP_SIZE SIZE_IP_CHAR
#define CONFIG_DIRECTORY_MAX_SIZE 128
#define CONFIG_BUFFER_SIZE 512

typedef enum {
    CLIENT,
    SERVER
}Config_type;

typedef struct {
    bool is_defined;
    char ip[CONFIG_CHAR_IP_SIZE];
    int port;
} Config_server;

typedef struct {
    bool is_defined;
    char ip[CONFIG_CHAR_IP_SIZE];
    int local_port;
    int public_port;
} Config_user;

typedef struct {
    bool is_defined;

    bool user_consent;
    char path[CONFIG_BUFFER_SIZE];
} Config_history;

typedef struct {
    bool is_defined;
    char certificate[CONFIG_BUFFER_SIZE];
    char key[CONFIG_BUFFER_SIZE];
} Config_infos_ssl;

typedef struct {
    Config_user user;
    Config_server server;
    Config_history history;
    Config_infos_ssl config_ssl;
} Config_infos;

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
Config_infos *loadConfig(Config_type type);

/**
 * @brief delete the t_config structure and free memory
 *
 * @param config struct t_config
 */
void deinitConfig(Config_infos **config);

#endif // !__CONFIG_H__
