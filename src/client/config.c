#include <client/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool getConfigFilePath(char config_file[DIRECTORY_MAX_SIZE]) {
    char current_dir[DIRECTORY_MAX_SIZE];
    memset(config_file, 0, DIRECTORY_MAX_SIZE);
    if (getcwd(current_dir, DIRECTORY_MAX_SIZE) == NULL) {
        perror("Couldn't get current working directory");
        return false;
    }
    char *needle = strnstr(current_dir, "close-review", strnlen(current_dir, DIRECTORY_MAX_SIZE));
    if (needle == NULL) {
        fprintf(stderr, "Directory 'close-review' not found in the current path\n");
        return false;
    }
    needle += strlen("close-review");
    memset(needle, '\0', DIRECTORY_MAX_SIZE - (needle - current_dir));
    snprintf(needle, sizeof("/config.toml"), "/config.toml");
    strncpy(config_file, current_dir, strnlen(current_dir, DIRECTORY_MAX_SIZE));
    config_file[DIRECTORY_MAX_SIZE - 1] = '\0';
    return true;
}

bool setConfig(FILE *config, t_addr *user, t_addr *server, t_history *history,
               t_conf_ssl *conf_ssl) {
    char header[BUFFER_SIZE];
    char token[BUFFER_SIZE];

    while (fgets(header, sizeof(header), config)) {
        if (sscanf(header, "[ %[^]] ]", token) == 1) {
            if (strncmp(token, "user", BUFFER_SIZE) == 0) {
                if (fscanf(config, "ip = \"%15[^\"]\"\n", user->ip) != 1) {
                    // User Configuration isn't mandatory
                    user->ip[0] = '\0';
                    user->port = -1;
                } else if (fscanf(config, "port = %d\n", &user->port) != 1) {
                    perror("Error reading user section");
                    return false;
                }
            } else if (strncmp(token, "server", BUFFER_SIZE) == 0) {
                if (fscanf(config, "ip = \"%15[^\"]\"\nport = %d\n", server->ip, &server->port) !=
                    2) {
                    perror("Error reading user section");
                    return false;
                }
            } else if (strncmp(token, "history", BUFFER_SIZE) == 0) {
                char consent[BUFFER_SIZE];
                if (fscanf(config, "user_consent = %s\npath = \"%[^\"]\"\n", consent,
                           history->path) != 2) {
                    perror("Error reading user section");
                    return false;
                }
                history->user_consent = (strcmp(consent, "true") == 0);
            } else if (strncmp(token, "ssl", BUFFER_SIZE) == 0) {
                if (fscanf(config, "certificate = \"%[^\"]\"\nkey = \"%[^\"]\"\n",
                           conf_ssl->certificate, conf_ssl->key) != 2) {
                    perror("Error reading user section");
                    return false;
                }
            }
        }
    }

    if (ferror(config)) {
        fclose(config);
        perror("Error reading config file");
        return false;
    }

    return true;
}
