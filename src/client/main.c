#include <client/config.h>
#include <client/tui.h>
#include <stdlib.h>

int main(void) {
    char config_file[DIRECTORY_MAX_SIZE];
    if (!getConfigFilePath(config_file)) {
        exit(EXIT_FAILURE);
    }

    printf("%s\n", config_file);

    FILE *config;
    if ((config = fopen(config_file, "r")) == NULL) {
        perror("Couldn't open the Config File");
        exit(EXIT_FAILURE);
    }

    t_addr_serv server;
    t_addr_user user;
    t_history history;
    t_conf_ssl conf_ssl;

    if (!setConfig(config, &user, &server, &history, &conf_ssl)) {
        exit(EXIT_FAILURE);
    }

    printf("User IP: %s\n", user.ip);
    printf("User Local Port: %d\n", user.local_port);
    printf("User Public Port: %d\n", user.public_port);
    printf("Server IP: %s\n", server.ip);
    printf("Server Port: %d\n", server.port);
    printf("History User Consent: %s\n", history.user_consent ? "true" : "false");
    printf("History Path: %s\n", history.path);
    printf("SSL Certificate Path: %s\n", conf_ssl.certificate);
    printf("SSL Key Path: %s\n", conf_ssl.key);

    fclose(config);

    return EXIT_SUCCESS;
}
