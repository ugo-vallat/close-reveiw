#ifndef __PROJECT_CONSTANTS_H__
#define __PROJECT_CONSTANTS_H__

/* Type Msg */
#define SIZE_DATE 20
#define SIZE_TIME 20
#define SIZE_MSG_DATA 2048
#define SIZE_NAME 30

/* server */
#define MAX_ONLINE 8

/* network */
#define SIZE_IP_CHAR 16

/* Type TXT */
#define SIZE_TXT 2048

/* Security */
#define MAX_PASSWORD_LENGTH 30
#define MIN_PASSWORD_LENGTH 8
#define SIZE_PASSWORD MAX_PASSWORD_LENGTH
#define SIZE_HASH 256

/* SQL */
#define SIZE_QUERY 512

/* Command */
#define SIZE_CMD_RES 1024
#define SIZE_MAX_CMD 32
#define NB_COMMANDS 9

/* TUI */
#define INPUT_HEIGHT 3

#define SIZE_DIRECTORY 256
#define SIZE_ERROR_CHAR 256

/* paths */
#define PATH_LOG "./logs.log"
#define PATH_CONFIG_CLIENT "./config/client/config.toml"
#define PATH_CONFIG_SERVER "./config/server/config.toml"


#endif
