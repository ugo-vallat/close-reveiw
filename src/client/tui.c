#include <client/tui.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>

Command stringToCommand(char command[COMMAND_MAX_SIZE]) {
    if (strncmp(command, "q", COMMAND_MAX_SIZE) == 0)
        return QUIT;
    return UNKNOWN;
}

void startTUI(void) {
    initscr();
    clear();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(25);

    int c;
    bool exited = false;
    char command[COMMAND_MAX_SIZE] = {0};
    Mode mode = NORMAL_MODE;

    while (!exited) {
        switch (mode) {
        case NORMAL_MODE:
            c = getch();
            switch (c) {
            case ':':
                mode = COMMAND_MODE;
                mvprintw(LINES - 1, 0, ":");
                clrtoeol();
                refresh();
                break;
            case '/':
                mvprintw(LINES - 1, 0, "/");
                clrtoeol();
                refresh();
                mode = SEARCH_MODE;
                break;
            case 'i':
                mvprintw(LINES - 1, 0, "-- INSERT --");
                clrtoeol();
                refresh();
                mode = INSERT_MODE;
                break;
            default:
                break;
            }
            break;

        case COMMAND_MODE:
            echo();
            getnstr(command, COMMAND_MAX_SIZE);
            noecho();
            mvprintw(LINES - 1, 0, "");
            clrtoeol();
            refresh();
            switch (stringToCommand(command)) {
            case QUIT:
                exited = true;
                break;
            case UNKNOWN:
                mvprintw(LINES - 1, 0, "Error: Not a valid command!");
                clrtoeol();
                refresh();
                break;
            }
            mode = NORMAL_MODE;
            break;

        case INSERT_MODE:
            while ((c = getch()) != ESCAPE_KEY) {
                // TODO: Implement INSERT_MODE functionality
                // Display the typed character for now
                addch(c);
                refresh();
            }
            mvprintw(LINES - 1, 0, "");
            clrtoeol();
            refresh();
            mode = NORMAL_MODE;
            break;

        case SEARCH_MODE:
            while ((c = getch()) != ESCAPE_KEY) {
                // TODO: Implement SEARCH_MODE functionality
                // Display the typed character for now
                addch(c);
                refresh();
            }
            mvprintw(LINES - 1, 0, "");
            clrtoeol();
            refresh();
            mode = NORMAL_MODE;
            break;
        }
    }

    endwin();
}
