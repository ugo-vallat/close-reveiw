
/**
 * @file genericlist.c
 * @author VALLAT Ugo
 *
 * @brief Cette librairie implémente une liste générique pseudo statique d'entiers
 *
 * Implémente la liste sous forme d'un tableau statique et alloue
 * de la mémoire dynamiquement lorsque qu'il est plein
 *
 * La liste ne contient que des pointeur génériques vers la donnée (void*)
 *
 * @note haute performance en lecture( O(1) ) mais faible en écriture ( O(n))
 *
 * @remark En cas d'erreur, toutes les fonctions de list exit le progamme avec un
 * message d'erreur
 */

// #define _POSIX_C_SOURCE 200809L
// #define _GNU_SOURCE

#include "network/tls-com.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <types/clientlist.h>
#include <types/genericlist.h>
#include <utils/logger.h>

#define FILE_NAME "clientlist.c"

/*-----------------------------------------------------------------*/
/*                       UTILS                                     */
/*-----------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/*                     STRUCTURE LIST GENERIC                       */
/*------------------------------------------------------------------*/

/**
 * @author Ugo VALLAT
 * @brief  Définition de la structure list
 */
struct s_client_list {
    unsigned memory_size;  /* Taille du tableau en mémoire */
    unsigned size;         /* taille de la liste (nombre éléments) */
    pthread_mutex_t mutex; /* mutex d'accès à la liste */
    Client **tab;          /* tableau des poiteur vers de client */
};

/**
 * @author Ugo VALLAT
 */
ClientList *initClientList(unsigned memory_size) {
    ClientList *l = malloc(sizeof(ClientList));
    if (l == NULL)
        exitl("genericlist.c", "createClientList", EXIT_FAILURE, "erreur malloc list");

    l->tab = malloc(sizeof(void *) * memory_size);
    if (l->tab == NULL)
        exitl("genericlist.c", "createClientList", EXIT_FAILURE, "erreur malloc tab");

    l->memory_size = memory_size;
    l->size = 0;
    if (pthread_mutex_init(&(l->mutex), NULL) != 0) {
        exitl("genericlist.c", "createClientList", EXIT_FAILURE, "erreur init mutex");
    }
    return l;
}

/**
 * @author Ugo VALLAT
 */
void deinitClientList(ClientList **l) { // TODO
    /* test l != NULL */
    // // testArgNull(l, "genericlist.c", "deleteClientList", "l");
    // // testArgNull((*l), "genericlist.c", "deleteClientList", "*l");

    /* libération de la mémoire */
    unsigned size = clientListSize((ClientList *)(*l));
    Client *temp;
    for (unsigned i = 0; i < size; i++) {
        temp = clientListPop((ClientList *)(*l));
        if(temp->info_user != NULL)
            tlsCloseCom(temp->info_user, NULL);
        if(temp->request_by != NULL)
            deinitGenList(&temp->request_by, free);
        free(temp);
    }
    free((*l)->tab);
    free((*l));
    *l = NULL;
}

/**
 * @author Ugo VALLAT
 * @brief Modifie l'espace mémoire aloué au tableau
 *
 * @param l Pointeur vers la liste
 * @param new_size Nouvelle taille du tableau
 * @pre l != NULL
 */
void adjustMemorySizeClientList(ClientList *l, unsigned new_size) {
    // testArgNull(l, "genericlist.c", "adjustMemorySizeClientList", "l");

    /* nouvelle taille de la liste */
    l->memory_size = new_size;

    /* modification taille du tableau */
    l->tab = realloc(l->tab, new_size * sizeof(void *));
    if (new_size != 0 && l->tab == NULL)
        exitl("genericlist.c", "adjustMemorySizeClientList", EXIT_FAILURE, "echec realloc tab");
}

/**
 * @author Ugo VALLAT
 */
void clientListAdd(ClientList *l, Client *c) {
    // // testArgNull(l, "genericlist.c", "clientListAdd", "l");
    pthread_mutex_lock(&(l->mutex));
    /* agrandissement de la liste si pleine */
    if (l->size == l->memory_size)
        adjustMemorySizeClientList(l, l->memory_size + 8);

    /* Ajout de la valeur */
    int i = 0;
    while (i < l->size && l->tab[i]->id < c->id) {
        if (l->tab[i]->id == c->id) {
            warnl(FILE_NAME, "cleintListAdd", "collision: ajoue d'un id deja present");
        }
        i++;
    }
    l->tab[l->size] = c;
    l->size++;
    pthread_mutex_unlock(&(l->mutex));
}

/**
 * @author Ugo VALLAT
 */
Client *clientListPop(ClientList *l) {
    /* vérification paramêtre */
    // // testArgNull(l, "genericlist.c", "listPop", "l");
    pthread_mutex_lock(&(l->mutex));
    if (l->size <= 0)
        exitl("list.c", "listPop", EXIT_FAILURE, "liste déjà vide");

    /* suppression de l'élément */
    void *elem = l->tab[l->size - 1];
    l->size--;
    adjustMemorySizeClientList(l, l->size);
    pthread_mutex_unlock(&(l->mutex));
    return elem;
}

/**
 * @author Ugo VALLAT
 */
Client *clientListRemove(ClientList *l, unsigned i) {
    /* vérification paramêtres */
    // // testArgNull(l, "genericlist.c", "clientListRemove", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "clientListRemove", EXIT_FAILURE, "position (%d) invalide", i);

    void *elem = l->tab[i];
    /* suppression de l'élément */
    for (int j = i; j < (int)l->size - 1; j++)
        l->tab[j] = l->tab[j + 1];
    l->size--;
    adjustMemorySizeClientList(l, l->size);
    pthread_mutex_unlock(&(l->mutex));
    return elem;
}

void clientListDelete(ClientList *l, unsigned i) {
    /* vérification paramêtres */
    // // testArgNull(l, "genericlist.c", "clientListRemove", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "clientListRemove", EXIT_FAILURE, "position (%d) invalide", i);

    Client *elem = l->tab[i];
    /* suppression de l'élément */
    for (int j = i; j < (int)l->size - 1; j++)
        l->tab[j] = l->tab[j + 1];
    l->size--;
    adjustMemorySizeClientList(l, l->size);
    pthread_mutex_unlock(&(l->mutex));
    deinitGenList(&elem->request_by, free);
    tlsCloseCom(elem->info_user, NULL);
    free(elem);
}

/**
 * @author Ugo VALLAT
 */
bool clientListIsEmpty(ClientList *l) {
    // // testArgNull(l, "genericlist.c", "listEmpty", "l");
    return l->size == 0;
}

/**
 * @author Ugo VALLAT
 */
unsigned clientListSize(ClientList *l) {
    // testArgNull(l, "genericlist.c", "clientListSize", "l");
    pthread_mutex_lock(&(l->mutex));
    unsigned size = l->size;
    pthread_mutex_unlock(&(l->mutex));
    return size;
}

/**
 * @author Ugo VALLAT
 */
Client *clientListGet(ClientList *l, unsigned i) {
    /* vérification paramêtre */
    // testArgNull(l, "genericlist.c", "clientListGet", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "clientListGet", EXIT_FAILURE, "position (%d) invalide", i);
    Client *ret = l->tab[i];
    pthread_mutex_unlock(&(l->mutex));
    return ret;
}

Client *clientListGetId(ClientList *l, int id) {
    /* vérification paramêtre */
    if (l == NULL) {
        fprintf(stderr, "Error: ClientList pointer is NULL\n");
        return NULL;
    }

    pthread_mutex_lock(&(l->mutex));
    printf(">>>>>>>>>>>>>>>>>>> ici\n");

    int i = 0; // Initialize the counter
    while (i < l->size) {
        if (l->tab[i] != NULL && l->tab[i]->id == id) {
            Client *ret = l->tab[i];
            printf(">>>>>>>>>>>>>fin ici\n");
            pthread_mutex_unlock(&(l->mutex));
            return ret;
        }
        i++;
    }

    printf("Client with id %d not found\n", id);
    pthread_mutex_unlock(&(l->mutex));
    return NULL;
}

/**
 * @author Ugo VALLAT
 */
void clientListSet(ClientList *l, Client *c, unsigned i) {
    /* vérification paramêtre */
    // testArgNull(l, "genericlist.c", "clientListSet", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "clientListSet", EXIT_FAILURE, "position (%d) invalide", i);

    l->tab[i] = c;
    pthread_mutex_unlock(&(l->mutex));
}
