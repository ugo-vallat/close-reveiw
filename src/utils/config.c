#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <utils/config.h>
#include <utils/logger.h>
#include <utils/project_constants.h>

#define FILE_CONFIG "config.c"

void deinitConfig(Config_infos **config) {
    free(*config);
    *config = NULL;
}

bool getConfigFilePath(char config_file[CONFIG_DIRECTORY_MAX_SIZE]) {
    char FUN_NAME[32] = "getConfigFilePath";
    char current_dir[CONFIG_DIRECTORY_MAX_SIZE];
    memset(config_file, 0, CONFIG_DIRECTORY_MAX_SIZE);
    if (getcwd(current_dir, CONFIG_DIRECTORY_MAX_SIZE) == NULL) {
        warnl(FILE_CONFIG, FUN_NAME, "Couldn't get current working directory");
        return false;
    }
    char *needle = strstr(current_dir, "close-review");
    if (needle == NULL) {
        warnl(FILE_CONFIG, FUN_NAME, "Directory 'close-review' not found in the current path");
        return false;
    }
    needle += strlen("close-review");
    memset(needle, '\0', CONFIG_DIRECTORY_MAX_SIZE - (needle - current_dir));
    snprintf(needle, sizeof("/config.toml"), "/config.toml");
    strncpy(config_file, current_dir, strnlen(current_dir, CONFIG_DIRECTORY_MAX_SIZE));
    config_file[CONFIG_DIRECTORY_MAX_SIZE - 1] = '\0';
    return true;
}

Config_infos *loadConfig(Config_type type) {
    char FUN_NAME[32] = "initConfig";
    char header[CONFIG_BUFFER_SIZE];
    char token[CONFIG_BUFFER_SIZE];
    FILE *config;

    switch (type) {
    case CLIENT:
        /* open config file */
        config = fopen(PATH_CONFIG_CLIENT, "r");
        if (config == NULL) {
            warnl(FILE_CONFIG, FUN_NAME, "failed to open config file at <%s>", PATH_CONFIG_CLIENT);
            return NULL;
        }
        break;
    case SERVER:
        /* open config file */
        config = fopen(PATH_CONFIG_SERVER, "r");
        if (config == NULL) {
            warnl(FILE_CONFIG, FUN_NAME, "failed to open config file at <%s>", PATH_CONFIG_SERVER);
            return NULL;
        }
        break;
    }

    /* malloc struct t_config */
    Config_infos *configuration = malloc(sizeof(Config_infos));
    if (configuration == NULL) {
        warnl(FILE_CONFIG, FUN_NAME, "couldn't allocate memory for config struct");
        fclose(config);
        return NULL;
    }
    bzero(configuration, sizeof(Config_infos));

    /* read config file */
    while (fgets(header, sizeof(header), config)) {
        if (sscanf(header, "[ %[^]] ]", token) == 1) {
            if (strncmp(token, "user", CONFIG_BUFFER_SIZE) == 0) {
                configuration->user.is_defined =
                    fscanf(config, "ip = \"%15[^\"]\"\n", configuration->user.ip) == 1 &&
                    fscanf(config, "local_port = %d\n", &configuration->user.local_port) == 1 &&
                    fscanf(config, "public_port = %d\n", &configuration->user.public_port) == 1;
                if (!configuration->user.is_defined) {
                    warnl(FILE_CONFIG, FUN_NAME, "Error reading user section");
                    configuration->user.ip[0] = '\0';
                    configuration->user.local_port = -1;
                    configuration->user.public_port = -1;
                }
            } else if (strncmp(token, "server", CONFIG_BUFFER_SIZE) == 0) {
                configuration->server.is_defined =
                    fscanf(config, "ip = \"%15[^\"]\"\n", configuration->server.ip) == 1 &&
                    fscanf(config, "port = %d\n", &configuration->server.port) == 1;
                if (!configuration->server.is_defined) {
                    warnl(FILE_CONFIG, FUN_NAME, "Error reading server section");
                    configuration->server.ip[0] = '\0';
                    configuration->server.port = -1;
                }
            } else if (strncmp(token, "history", CONFIG_BUFFER_SIZE) == 0) {
                char consent[CONFIG_BUFFER_SIZE];
                configuration->history.is_defined =
                    fscanf(config, "user_consent = %s\n", consent) == 1 &&
                    fscanf(config, "path = \"%[^\"]\"\n", configuration->history.path) == 1;
                if (!configuration->history.is_defined) {
                    warnl(FILE_CONFIG, FUN_NAME, "Error reading history section");
                    configuration->history.user_consent = false;
                    configuration->history.path[0] = '\0';
                }
                configuration->history.user_consent = (strcmp(consent, "true") == 0);
            } else if (strncmp(token, "ssl", CONFIG_BUFFER_SIZE) == 0) {
                configuration->config_ssl.is_defined =
                    fscanf(config, "certificate = \"%[^\"]\"\n", configuration->config_ssl.certificate) == 1 &&
                    fscanf(config, "key = \"%[^\"]\"\n", configuration->config_ssl.key) == 1;
                if (!configuration->config_ssl.is_defined) {
                    warnl(FILE_CONFIG, FUN_NAME, "Error reading ssl section");
                    configuration->config_ssl.certificate[0] = '\0';
                    configuration->config_ssl.key[0] = '\0';
                }
            }
        }
    }

    if (ferror(config) && !feof(config)) {
        warnl(FILE_CONFIG, FUN_NAME, "Error reading config file");
    }
    fclose(config);

    return configuration;
}
