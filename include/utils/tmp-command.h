#ifndef __TMP_COMMAND_H__
#define __TMP_COMMAND_H__

#include <utils/genericlist.h>

#define SIZE_CMD_RES 1024

typedef enum e_type_cmd { NONE } Type_cmd;

typedef struct s_command {
    Type_cmd cmd;
    char res[SIZE_CMD_RES];
} Command;

#endif // !__TMP_COMMAND_H__