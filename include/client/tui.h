#ifndef __TUI_H__
#define __TUI_H__

#define COMMAND_MAX_SIZE 32
#define ESCAPE_KEY 27

typedef enum { NORMAL_MODE, COMMAND_MODE, INSERT_MODE, SEARCH_MODE } Mode;

typedef enum { QUIT, UNKNOWN } Command;

void startTUI(void);

#endif // !__TUI_H__
