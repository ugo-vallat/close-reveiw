#include <client/tui_v2.h>
#include <curses.h>
#include <menu.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NB_FRIENDS 4
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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

void menu(void) {
    initscr();
    clear();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    char friends[NB_FRIENDS][32] = {"Matéo", "Ugo", "Corentin", "Benoît"};
    char descriptions[NB_FRIENDS][64];
    int n = ARRAY_SIZE(friends);
    ITEM **items = calloc(n + 1, sizeof(ITEM *));
    for (int i = 0; i < n; i++) {
        items[i] = new_item(friends[i], descriptions[i]);
    }
    items[NB_FRIENDS] = (ITEM *)NULL;

    MENU *menu = new_menu((ITEM **)items);

    post_menu(menu);
    refresh();

    int c;
    while ((c = getch()) != 'q') {
        switch (c) {
        case KEY_DOWN:
            menu_driver(menu, REQ_DOWN_ITEM);
            break;
        case KEY_UP:
            menu_driver(menu, REQ_UP_ITEM);
            break;
        case 10:
            mvprintw(LINES - 2, 0, "You chose: %s", item_name(current_item(menu)));
            clrtoeol();
            refresh();
            break;
        }
        refresh();
    }

    unpost_menu(menu);
    free_menu(menu);

    for (int i = 0; i < NB_FRIENDS; i++) {
        free_item(items[i]);
    }

    endwin();
}
