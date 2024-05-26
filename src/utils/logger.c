#include <errno.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/logger.h>

#define YELLOW "\033[38;5;184m"
#define ORANGE "\033[38;5;208m"
#define RED "\033[38;5;160m"
#define RSTC "\033[0m"

FILE *output = NULL;
bool console;
char *c_yellow;
char *c_orange;
char *c_rstc;

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 */
void init_logger(const char *file_path) {
    if (file_path) {
        output = fopen(file_path, "a");
        console = false;
        c_yellow = "";
        c_orange = "";
        c_rstc = "";
    } else {
        output = stdout;
        console = true;
        c_yellow = YELLOW;
        c_orange = ORANGE;
        c_rstc = RSTC;
    }
}

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 */
void printl(const char *format, ...) {
#ifdef DEBUG
    if (output == NULL) {
        fprintf(stderr, "Warning: logger call to printl while being not initialized!");
        return;
    }
#endif
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    fprintf(output, "\n");
    fflush(output); // intégrité des logs
    va_end(args);
}

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 */
void warnl(const char *file_name, const char *fun_name, const char *format, ...) {
#ifdef DEBUG
    if (output == NULL) {
        fprintf(stderr,
                "Warning: logger call to warnl while being not initialized!\nlast "
                "call was from %s in %s\n",
                fun_name, file_name);
        return;
    }
#endif

    va_list args;
    va_start(args, format);
    if (errno) {
        perror("Warnl with errno ");
        errno = 0;
    }
    // format de sortie dépendant
    if (console) {
        fprintf(output, YELLOW);
        fprintf(output, "[warnl] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, RSTC);
        fprintf(output, "\n");
    } else {
        fprintf(output, "[warnl] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, "\n");
    }
    fflush(output); // intégrité des logs
    va_end(args);
}

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 * @brief Affiche la pile d'appel de fonction
 * @pre Drapeau de compilation -rdynamic
 */
void printStackTrace() {
    void *buffer[64];
    int nbv = backtrace(buffer, sizeof(buffer));
    char **strings = backtrace_symbols(buffer, nbv);
    for (int i = 1; i < nbv; i++) { // démarre à 1 pour ignorer l'appel de cette fonction
        fprintf(stderr, "%s\n", strings[i]);
    }
    free(strings);
}

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 */
void exitl(const char *file_name, const char *fun_name, int exit_value, const char *format, ...) {
#ifdef DEBUG
    printStackTrace();
    if (output == NULL) {
        fprintf(stderr, "Warning: logger call to exitl while being not initialized!\n");
        exit(EXIT_FAILURE);
    }
#endif
    va_list args;
    va_start(args, format);
    if (errno) {
        perror("Exit with errno ");
        errno = 0;
    }
    // format de sortie dépendant
    if (console) {
        fprintf(output, RED);
        fprintf(output, "[exitl] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, RSTC);
        fprintf(output, "\n");
    } else {
        fprintf(output, "[exitl] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, "\n");
    }
    va_end(args);
    close_logger();
    exit(exit_value);
}

void assertl(bool assert, const char *file_name, const char *fun_name, int exit_value, const char *format, ...) {
    va_list args;
    /* If assertion is true, nothing to do */
    if (assert)
        return;

    /* else */
    va_start(args, format);
    if (errno) {
        perror("Assert with errno ");
        errno = 0;
    }
    // format de sortie dépendant
    if (console) {
        fprintf(output, RED);
        fprintf(output, "[assert] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, RSTC);
        fprintf(output, "\n");
    } else {
        fprintf(output, "[assert] %s > %s : ", file_name, fun_name);
        vfprintf(output, format, args);
        fprintf(output, "\n");
    }
    va_end(args);
    close_logger();
    exit(exit_value);
}

/**
 * @date 04/11/2023
 * @author LAFORGE Mateo
 */
void close_logger(void) {
    if (output != NULL && output != stdout)
        fclose(output);
}
