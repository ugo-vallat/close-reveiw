
/**
 * @file genericlist.h
 * @author VALLAT Ugo
 *
 * @brief Cette librairie implémente une liste générique pseudo statique d'entiers
 *
 * Implémente la liste sous forme d'un tableau statique et alloue
 * de la mémoire dynamiquement lorsque qu'il est plein
 *
 * La liste générique est thread-safe (sauf delete)
 *
 * La liste ne contient que des pointeur génériques vers la donnée (void*)
 *
 * @note haute performance en lecture( O(1) ) mais faible en écriture ( O(n))
 *
 * @remark En cas d'erreur, toutes les fonctions de liste générique exit le progamme avec un
 * message d'erreur
 */

#ifndef __GENLIST_H__
#define __GENLIST_H__
#include <stdbool.h>

/*------------------------------------------------------------------*/
/*                     STRUCTURE LIST GENERIC                       */
/*------------------------------------------------------------------*/

/* Définition opaque de la structure list */
typedef struct s_gen_list GenList;
typedef GenList *ptrGenList;
typedef void (*freefun)(void *);

/**
 * @brief Crée une liste vide
 *
 * @param[in] memory_size Espace mémoire initial (en nombre d'éléments)
 *
 * @return pointeur vers la liste
 * @note Alloue la mémoire mais n'est pas initialisée (taille liste = 0)
 */
GenList *createGenList(unsigned memory_size);

/**
 * @brief Supprime la liste mais ne supprime pas la donées pointée
 * @pre l != NULL
 * @pre *l != NULL
 *
 * @param[in] l liste à supprimer
 */
void deleteGenList(ptrGenList *l, freefun fun);

/*
 @brief Supprume les elment de la liste

*/
void clearGenList(GenList *l);

/**
 * @brief Ajoute l'élément à la fin de la liste
 *
 * @param[in] l Pointeur vers la liste
 * @param[in] v Valeur à ajouter
 * @pre l != NULL
 */
void genListAdd(GenList *l, void *v);

/**
 * @brief Insert une valeur à la position i
 *
 * @param[in] l Pointeur vers la liste
 * @param[in] v Valeur à ajouter
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i <= listSize
 */
void genListInsert(GenList *l, void *v, unsigned i);

/**
 * @brief Supprime le dernier élément de la liste
 *
 * @param[in] l list
 * @pre l != NULL
 *
 * @pre taille liste > 0
 * @return Valeur avant supression
 **/
void *genListPop(GenList *l);

/**
 * @brief Supprime l'élément à la position i
 *
 * @param[in] l Pointeur vers la liste
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i < listSize
 * @return Valeur avant supression
 */
void *genListRemove(GenList *l, unsigned i);

/**
 * @brief Lire la valeur à la position i
 *
 * @param[in] l Pointeur vers la liste
 * @param[in] i Position de l'élément
 *
 * @pre l != NULL
 * @pre i < list size
 *
 * @return Valeur lue
 **/
void *genListGet(GenList *l, unsigned i);

/**
 * @author VALLAT Ugo
 * @brief Change la valeur à la position i par une nouvelle valeur
 *
 * @param[in] l Pointeur vers la liste
 * @param[in] v Nouvelle valeur
 * @param[in] i Position
 * @pre l != NULL
 */
void genListSet(GenList *l, void *v, unsigned i);

/**
 *
 * @brief Renvoie si la liste est vide
 *
 * @param[in] l Pointeur vers la liste
 * @pre l != NULL
 *
 * @return true si vide, false sinon
 */
bool genListIsEmpty(GenList *l);

/**
 * @author LAFORGE Mateo
 * @brief Cherche un élément e dans une liste l et renvoie un booléen correspondant
 *
 * @param[in] l la liste dans laquelle chercher
 * @param[in] e l'élément à chercher
 *
 * @return true si e est dans l, false sinon
 */
bool genListContains(GenList *l, void *e);

/**
 * @brief Renvoie la taille de la liste (position + 1 du dernier élément)
 *
 * @param[in] l Pointeur vers la liste
 * @pre l != NULL
 *
 * @return taille de la liste
 *
 */

unsigned genListSize(GenList *l);

/**
 * @brief Copie la liste en entrée
 *
 * @param[in] l Pointeur de la liste à copier
 * @pre l != NULL
 *
 * @return  Pointeur vers la copie
 *
 */
GenList *genListCopy(GenList *l);

#endif