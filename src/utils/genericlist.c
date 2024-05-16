
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

#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <utils/genericlist.h>
#include <utils/logger.h>

/*-----------------------------------------------------------------*/
/*                       UTILS                                     */
/*-----------------------------------------------------------------*/

void testArgNull(void *arg, char *file, char *fun, char *name) {
    if (arg == NULL)
        exitl(file, "hisArgNull", EXIT_FAILURE, "[%s in %s] %s : argument NULL\n", fun, file, name);
}

/*------------------------------------------------------------------*/
/*                     STRUCTURE LIST GENERIC                       */
/*------------------------------------------------------------------*/

/**
 * @author Ugo VALLAT
 * @brief  Définition de la structure list
 */
struct s_gen_list {
    unsigned memory_size;  /* Taille du tableau en mémoire */
    unsigned size;         /* taille de la liste (nombre éléments) */
    pthread_mutex_t mutex; /* mutex d'accès à la liste */
    void **tab;            /* tableau des poiteur (générique) */
};

/**
 * @author Ugo VALLAT
 */
GenList *initGenList(unsigned memory_size) {
    GenList *l = malloc(sizeof(GenList));
    if (l == NULL)
        exitl("genericlist.c", "createGenList", EXIT_FAILURE, "erreur malloc list");

    l->tab = malloc(sizeof(void *) * memory_size);
    if (l->tab == NULL)
        exitl("genericlist.c", "createGenList", EXIT_FAILURE, "erreur malloc tab");

    l->memory_size = memory_size;
    l->size = 0;
    if (pthread_mutex_init(&(l->mutex), NULL) != 0) {
        exitl("genericlist.c", "createGenList", EXIT_FAILURE, "erreur init mutex");
    }
    return l;
}

/**
 * @author Ugo VALLAT
 */
void deinitGenList(ptrGenList *l, freefun fun) {
    /* test l != NULL */
    testArgNull(l, "genericlist.c", "deleteGenList", "l");
    testArgNull((*l), "genericlist.c", "deleteGenList", "*l");

    /* libération de la mémoire */
    while (genListSize((GenList *)(*l))) {
        fun(genListPop((GenList *)(*l)));
    }
    free((*l)->tab);
    free((*l));
    *l = NULL;
}

void genListClear(GenList *l, freefun fun) {
    testArgNull(l, "genericlist.c", "clearGenList", "l");
    pthread_mutex_lock(&(l->mutex));

    while (!genListIsEmpty(l)) {
        fun(genListPop(l));
    }
    pthread_mutex_unlock(&(l->mutex));
}

/**
 * @author Ugo VALLAT
 * @brief Modifie l'espace mémoire aloué au tableau
 *
 * @param l Pointeur vers la liste
 * @param new_size Nouvelle taille du tableau
 * @pre l != NULL
 */
void adjustMemorySizeGenList(GenList *l, unsigned new_size) {
    testArgNull(l, "genericlist.c", "adjustMemorySizeGenList", "l");

    /* nouvelle taille de la liste */
    l->memory_size = new_size;

    /* modification taille du tableau */
    l->tab = realloc(l->tab, new_size * sizeof(void *));
    if (new_size != 0 && l->tab == NULL)
        exitl("genericlist.c", "adjustMemorySizeGenList", EXIT_FAILURE, "echec realloc tab");
}

/**
 * @author Ugo VALLAT
 */
void genListAdd(GenList *l, void *v) {
    testArgNull(l, "genericlist.c", "genListAdd", "l");
    pthread_mutex_lock(&(l->mutex));
    /* agrandissement de la liste si pleine */
    if (l->size == l->memory_size)
        adjustMemorySizeGenList(l, l->memory_size + 8);

    /* Ajout de la valeur */
    l->tab[l->size] = v;
    l->size++;
    pthread_mutex_unlock(&(l->mutex));
}

/**
 * @author Ugo VALLAT
 */
void genListInsert(GenList *l, void *v, unsigned i) {
    /* vérification paramêtres */
    testArgNull(l, "genericlist.c", "genListInsert", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i > l->size)
        exitl("genericlist.c", "genListInsert", EXIT_FAILURE, "position (%d) invalide", i);

    /* agrandissement de la liste si pleine */
    if (l->size >= l->memory_size)
        adjustMemorySizeGenList(l, l->memory_size + 8);

    /* décale tous les éléments */
    for (int j = l->size - 1; j >= (int)i; j--)
        l->tab[j + 1] = l->tab[j];

    /* ajoute le nouvel élément */
    l->tab[i] = v;
    l->size++;
    pthread_mutex_unlock(&(l->mutex));
}

/**
 * @author Ugo VALLAT
 */
void *genListPop(GenList *l) {
    /* vérification paramêtre */
    testArgNull(l, "genericlist.c", "listPop", "l");
    pthread_mutex_lock(&(l->mutex));
    if (l->size <= 0)
        exitl("list.c", "listPop", EXIT_FAILURE, "liste déjà vide");

    /* suppression de l'élément */
    void *elem = l->tab[l->size - 1];
    l->size--;
    adjustMemorySizeGenList(l, l->size);
    pthread_mutex_unlock(&(l->mutex));
    return elem;
}

/**
 * @author Ugo VALLAT
 */
void *genListRemove(GenList *l, unsigned i) {
    /* vérification paramêtres */
    testArgNull(l, "genericlist.c", "genListRemove", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "genListRemove", EXIT_FAILURE, "position (%d) invalide", i);

    void *elem = l->tab[i];
    /* suppression de l'élément */
    for (int j = i; j < (int)l->size - 1; j++)
        l->tab[j] = l->tab[j + 1];
    l->size--;
    adjustMemorySizeGenList(l, l->size);
    pthread_mutex_unlock(&(l->mutex));
    return elem;
}

/**
 * @author Ugo VALLAT
 */
bool genListIsEmpty(GenList *l) {
    testArgNull(l, "genericlist.c", "listEmpty", "l");

    return l->size == 0;
}

/**
 * @author LAFORGE Mateo
 */
bool genListContains(GenList *l, void *e) {
    testArgNull(l, "genericlist.c", "listContains", "l");
    pthread_mutex_lock(&(l->mutex));
    for (unsigned int i = 0; i < genListSize(l); i++)
        if (genListGet(l, i) == e) {
            pthread_mutex_unlock(&(l->mutex));
            return true;
        }
    pthread_mutex_unlock(&(l->mutex));
    return false;
}

/**
 * @author Ugo VALLAT
 */
unsigned genListSize(GenList *l) {
    testArgNull(l, "genericlist.c", "genListSize", "l");
    pthread_mutex_lock(&(l->mutex));
    unsigned size = l->size;
    pthread_mutex_unlock(&(l->mutex));
    return size;
}

/**
 * @author Ugo VALLAT
 */
GenList *genListCopy(GenList *l) {
    /* vérification paramêtre */
    testArgNull(l, "genericlist.c", "listCopy", "l");
    pthread_mutex_lock(&(l->mutex));

    /* création nouvelle liste */
    GenList *new = createGenList(l->size);

    /* copie des éléments */
    for (unsigned i = 0; i < l->size; i++)
        genListAdd(new, l->tab[i]);

    pthread_mutex_unlock(&(l->mutex));
    return new;
}

/**
 * @author Ugo VALLAT
 */
void *genListGet(GenList *l, unsigned i) {
    /* vérification paramêtre */
    testArgNull(l, "genericlist.c", "genListGet", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "genListGet", EXIT_FAILURE, "position (%d) invalide", i);
    void *ret = l->tab[i];
    pthread_mutex_unlock(&(l->mutex));
    return ret;
}

/**
 * @author Ugo VALLAT
 */
void genListSet(GenList *l, void *v, unsigned i) {
    /* vérification paramêtre */
    testArgNull(l, "genericlist.c", "genListSet", "l");
    pthread_mutex_lock(&(l->mutex));
    if (i >= l->size)
        exitl("genericlist.c", "genListSet", EXIT_FAILURE, "position (%d) invalide", i);

    l->tab[i] = v;
    pthread_mutex_unlock(&(l->mutex));
}

void genListPrintl(GenList *l, printGen fun) {
    testArgNull(l, "genericlist.c", "genListPrintl", "l");
    testArgNull(l, "genericlist.c", "genListPrintl", "fun");

    char *desc;
    pthread_mutex_lock(&(l->mutex));
    printl("{");
    for (unsigned i = 0; i < genListSize(l); i++) {
        desc = fun(l->tab[i]);
        printl("\t %s ,\n", desc);
        free(desc);
    }
    printl("}\n");
    pthread_mutex_unlock(&(l->mutex));
}