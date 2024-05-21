/**
 * @file list.c
 * @author VALLAT Ugo
 *
 * @brief Implémentation de la liste pseudo statique d'entiers
 *
 * Implémente la liste sous forme d'un tableau statique et alloue
 * de la mémoire dynamiquement lorsque qu'il est plein
 *
 * @remark En cas d'erreur, toutes les fonctions de list exit le progamme avec un
 * message d'erreur
 */

#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <types/list.h>
#include <utils/logger.h>

/*------------------------------------------------------------------*/
/*                        STRUCTURE LIST                            */
/*------------------------------------------------------------------*/

/* Définition de la structure list*/
struct s_list {
    unsigned memory_size; /* Taille du tableau en mémoire */
    unsigned size;        /* taille de la liste (nombre éléments) */
    LIST_TYPE *tab;       /* tableau des valeurs */
    pthread_mutex_t *mutex;
};

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
List *initList(unsigned memory_size) {
    List *l = malloc(sizeof(List));
    assertl(l, "list.c", "createList", EXIT_FAILURE, "erreur malloc list");

    l->tab = malloc(sizeof(LIST_TYPE) * memory_size);
    assertl(l->tab, "list.c", "createList", EXIT_FAILURE, "erreur malloc tab");

    l->memory_size = memory_size;
    l->size = 0;
    l->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(l->mutex, NULL);

    return l;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void deinitList(ptrList *l) {
    assertl(l, "list.c", "deleteList", EXIT_FAILURE, "l");
    assertl(*l, "list.c", "deleteList", EXIT_FAILURE, "*l");

    /* libération de la mémoire */
    pthread_mutex_destroy((*l)->mutex);
    free((*l)->mutex);
    free((*l)->tab);
    free((*l));
    *l = NULL;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 * @brief Modifie l'espace mémoire aloué au tableau
 *
 * @param l Pointeur vers la liste
 * @param new_size Nouvelle taille du tableau
 * @pre l != NULL
 */
void adjustMemorySizeList(List *l, unsigned new_size) {
    assertl(l, "list.c", "adjustMemorySizeList", EXIT_FAILURE, "l");

    /* nouvelle taille de la liste */
    l->memory_size = new_size;

    /* modification taille du tableau */
    l->tab = realloc(l->tab, new_size * sizeof(LIST_TYPE));
    if (new_size != 0 && l->tab == NULL)
        exitl("list.c", "adjustMemorySizeList", EXIT_FAILURE, "echec realloc tab");
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listAdd(List *l, LIST_TYPE v) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listAdd", EXIT_FAILURE, "l");

    /* agrandissement de la liste si pleine */
    if (l->size == l->memory_size)
        adjustMemorySizeList(l, l->memory_size + 8);

    /* Ajout de la valeur */
    l->tab[l->size] = v;
    l->size++;
    pthread_mutex_unlock(l->mutex);
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listInsert(List *l, LIST_TYPE v, unsigned i) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listInsert", EXIT_FAILURE, "l");
    assertl(i > l->size, "list.c", "listInsert", EXIT_FAILURE, "position (i) invalide");
    /* agrandissement de la liste si pleine */
    if (l->size >= l->memory_size)
        adjustMemorySizeList(l, l->memory_size + 8);

    /* décale tous les éléments */
    for (int j = l->size - 1; j >= (int)i; j--)
        l->tab[j + 1] = l->tab[j];

    /* ajoute le nouvel élément */
    l->tab[i] = v;
    l->size++;
    pthread_mutex_unlock(l->mutex);
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
LIST_TYPE listPop(List *l) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listPop", EXIT_FAILURE, "l");
    assertl(l->size > 0, "list.c", "listPop", EXIT_FAILURE, "liste déjà vide");
    /* suppression de l'élément */
    LIST_TYPE elem = l->tab[l->size - 1];
    l->size--;
    adjustMemorySizeList(l, l->size);
    pthread_mutex_unlock(l->mutex);
    return elem;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
LIST_TYPE listRemove(List *l, unsigned i) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listRemove", EXIT_FAILURE, "l");
    assertl(i < l->size, "list.c", "listRemove", EXIT_FAILURE, "position (i) invalide");
    LIST_TYPE elem = l->tab[i];

    /* suppression de l'élément */
    for (int j = i; j < (int)l->size - 1; j++)
        l->tab[j] = l->tab[j + 1];
    l->size--;
    adjustMemorySizeList(l, l->size);
    pthread_mutex_unlock(l->mutex);
    return elem;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
bool listIsEmpty(List *l) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listEmpty", EXIT_FAILURE, "l");
    unsigned size = l->size;
    pthread_mutex_unlock(l->mutex);
    return size == 0;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
unsigned listSize(List *l) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "lestSize", EXIT_FAILURE, "l");
    unsigned size = l->size;
    pthread_mutex_unlock(l->mutex);
    return size;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
List *listCopy(List *l) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listCopy", EXIT_FAILURE, "l");

    /* création nouvelle liste */
    List *new = initList(l->size);

    /* copie des éléments */
    for (unsigned i = 0; i < l->size; i++) {
        new->tab[i] = l->tab[i];
        new->size++;
    }
    pthread_mutex_unlock(l->mutex);
    return new;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
LIST_TYPE listGet(List *l, unsigned i) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listGet", EXIT_FAILURE, "l");
    assertl(i > l->size, "list.c", "listGet", EXIT_FAILURE, "position (%d) invalide", i);
    LIST_TYPE v = l->tab[i];
    pthread_mutex_unlock(l->mutex);
    return v;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listSet(List *l, LIST_TYPE v, unsigned i) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listSet", EXIT_FAILURE, "l");
    assertl(i > l->size, "list.c", "listSet", EXIT_FAILURE, "position (%d) invalide", i);
    l->tab[i] = v;
    pthread_mutex_unlock(l->mutex);
}

void listClear(List *l) {
    pthread_mutex_lock(l->mutex);
    assertl(l, "list.c", "listClear", EXIT_FAILURE, "l");
    l->size = 0;
    pthread_mutex_unlock(l->mutex);
}
