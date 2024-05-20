#ifndef __TUI_H__
#define __TUI_H__

#include <stdbool.h>
#include <types/packet.h>

#define SIZE_NAME 30
#define SIZE_INPUT 2048

typedef enum e_tui_error {
    TUI_SUCCESS = 0,                  /* Operation succeeded */
    TUI_INPUT_ERROR = -1,             /* Error reading input from the user */
    TUI_MEMORY_ALLOCATION_ERROR = -2, /* Failed to allocate memory */
    TUI_OUTPUT_FORMATTING_ERROR = -3, /* Failed to format packet before diplaying */
} TUI_error;

bool isValidUserId(char *user_id);

TUI_error stdinGetUserInput(char *buffer);

void *stdinHandler(void *arg);

TUI_error stdoutDisplayPacket(Packet *packet);

void *stdoutHandler(void *arg);

#endif // !__TUI_H__
