#ifndef __TUI_H__
#define __TUI_H__

#include <ncurses.h>
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

TUI_error stdinGetUserInput(char **buffer);

TUI_error stdinGetUserInput2(char *buffer, WINDOW *input_win);

void *stdinHandler(void *arg);

TUI_error stdoutDisplayPacket(Packet *packet);

TUI_error stdoutDisplayPacket2(Packet *packet, WINDOW *output_win);

void *stdoutHandler(void *arg);

void initWindows(WINDOW *output_win, WINDOW *input_win);

#endif // !__TUI_H__
