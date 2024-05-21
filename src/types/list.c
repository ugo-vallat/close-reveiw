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
    int *tab;             /* tableau des valeurs */
};

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
List *initList(unsigned memory_size) {
    List *l = malloc(sizeof(List));
    assertl(l == NULL, "list.c", "createList", EXIT_FAILURE, "erreur malloc list");

    l->tab = malloc(sizeof(int) * memory_size);
    assertl(l->tab == NULL, "list.c", "createList", EXIT_FAILURE, "erreur malloc tab");

    l->memory_size = memory_size;
    l->size = 0;
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
    l->tab = realloc(l->tab, new_size * sizeof(int));
    if (new_size != 0 && l->tab == NULL)
        exitl("list.c", "adjustMemorySizeList", EXIT_FAILURE, "echec realloc tab");
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listAdd(List *l, int v) {
    assertl(l, "list.c", "listAdd", EXIT_FAILURE, "l");

    /* agrandissement de la liste si pleine */
    if (l->size == l->memory_size)
        adjustMemorySizeList(l, l->memory_size + 8);

    /* Ajout de la valeur */
    l->tab[l->size] = v;
    l->size++;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listInsert(List *l, int v, unsigned i) {
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
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
int listPop(List *l) {
    assertl(l, "list.c", "listPop", EXIT_FAILURE, "l");
    assertl(l->size <= 0, "list.c", "listPop", EXIT_FAILURE, "liste déjà vide");
    /* suppression de l'élément */
    int elem = l->tab[l->size - 1];
    l->size--;
    adjustMemorySizeList(l, l->size);
    return elem;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
int listRemove(List *l, unsigned i) {
    assertl(l, "list.c", "listRemove", EXIT_FAILURE, "l");
    assertl(i >= l->size, "list.c", "listPop", EXIT_FAILURE, "position (i) invalide");
    int elem = l->tab[i];

    /* suppression de l'élément */
    for (int j = i; j < (int)l->size - 1; j++)
        l->tab[j] = l->tab[j + 1];
    l->size--;
    adjustMemorySizeList(l, l->size);

    return elem;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
bool listEmpty(List *l) {
    assertl(l, "list.c", "listEmpty", EXIT_FAILURE, "l");

    return l->size == 0;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
unsigned listSize(List *l) {
    assertl(l, "list.c", "lestSize", EXIT_FAILURE, "l");

    return l->size;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
List *listCopy(List *l) {
    assertl(l, "list.c", "listCopy", EXIT_FAILURE, "l");

    /* création nouvelle liste */
    List *new = initList(l->size);

    /* copie des éléments */
    for (unsigned i = 0; i < l->size; i++) {
        listAdd(new, l->tab[i]);
    }
    return new;
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
int listGet(List *l, unsigned i) {
    assertl(l, "list.c", "listGet", EXIT_FAILURE, "l");
    assertl(i > l->size, "list.c", "listGet", EXIT_FAILURE, "position (%d) invalide", i);
    return l->tab[i];
}

/**
 * @date  1/11/2023
 * @author Ugo VALLAT
 */
void listSet(List *l, int v, unsigned i) {
    assertl(l, "list.c", "listSet", EXIT_FAILURE, "l");
    assertl(i > l->size, "list.c", "listSet", EXIT_FAILURE, "position (%d) invalide", i);
    l->tab[i] = v;
}

void listClear(List *l) {
    assertl(l, "list.c", "listClear", EXIT_FAILURE, "l");
    l->size = 0;
}

