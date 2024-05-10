#include <client/history_manager.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <utils/message.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/logger.h>

#define MAX_SIZE_PATH 128

struct s_history_info{
    FILE *file;
};

bool directoryExists(const char *path) {
    struct stat dir_stat;
    if (stat(path, &dir_stat) == 0) {
        // Check if the path exists and is a directory
        if (S_ISDIR(dir_stat.st_mode)) {
            return 1; // Directory exists
        } else {
            return 0; // Path exists but is not a directory
        }
    } else {
        return 0; // Path does not exist or error occurred
    }
}

HistoryInfo *initHistory(char *path_dossier_historique, char *name_conversation){

    if(!directoryExists(path_dossier_historique)){
        warnl("history_manager", "initHistory", "Invalide path");
        return NULL;
    }

    char* dir = malloc(MAX_SIZE_PATH+SIZE_NAME);
    snprintf(dir,MAX_SIZE_PATH+SIZE_NAME,"%s/%s", path_dossier_historique, name_conversation);


    if(!directoryExists(dir)){
        mkdir(dir, S_IRWXU | S_IRWXG); //TODO voir quelle autorisation sont sense
    }

    char *f = malloc(MAX_SIZE_PATH+SIZE_NAME*2);
    snprintf(f,MAX_SIZE_PATH+SIZE_NAME*2,"%s/%s.txt", dir, name_conversation);
    FILE *file = fopen(f,"a+"); //ATTENTION a+ potenciellement casser

    if(file == NULL){
        warnl("history_manager", "initHistory", "file cannot be open");
        return NULL;
    }

    HistoryInfo *h = malloc(sizeof(HistoryInfo));

    h->file = file;

    return h;
}

void deinitHistory(HistoryInfo **info){
    if(fclose((*info)->file) != 0){
        warnl("history_manager", "deinitHistory", "error fclose");
    }
    free(*info);
    info=NULL;
}

int writeHistory(HistoryInfo *info, Msg *message){
    return fwrite(message, sizeof(char), sizeof(Msg), info->file);
}

