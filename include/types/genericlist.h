/**
 * @file genericlist.h
 *
 * @brief This library implements a pseudo static generic list of integers
 *
 * Implements the list in the form of a static array and allocates
 * dynamic memory when it is full
 *
 * The generic list is thread-safe (except delete)
 *
 * The list only contains generic pointers to the data (void*)
 *
 * @note high performance in reading (O(1)) but low in writing (O(n))
 *
 * @remark In case of error, all generic list functions exit the program with an
 * error message
 */

#ifndef __GENLIST_H__
#define __GENLIST_H__

#include <stdbool.h>

/*------------------------------------------------------------------*/
/*                     GENERIC LIST STRUCTURE                       */
/*------------------------------------------------------------------*/

/* Opaque definition of the list structure */
typedef struct s_gen_list GenList;
typedef GenList *ptrGenList;
typedef void (*freefun)(void *);

/**
 * @brief Creates an empty list
 *
 * @param[in] memory_size Initial memory space (in number of elements)
 *
 * @return pointer to the list
 * @note Allocates memory but is not initialized (list size = 0)
 */
GenList *initGenList(unsigned memory_size);

/**
 * @brief Deletes the list but does not delete the pointed data
 * @pre l != NULL
 * @pre *l != NULL
 *
 * @param[in] l list to delete
 */
void deinitGenList(ptrGenList *l, freefun fun);

/**
 * @brief Remove all elements from the list and free memory
 *
 * @param l List to clear
 * @param fun Function to free data
 */
void genListClear(GenList *l, freefun fun);

/**
 * @brief Adds the element to the end of the list
 *
 * @param[in] l Pointer to the list
 * @param[in] v Value to add
 * @pre l != NULL
 */
void genListAdd(GenList *l, void *v);

/**
 * @brief Inserts a value at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] v Value to add
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i <= listSize
 */
void genListInsert(GenList *l, void *v, unsigned i);

/**
 * @brief Removes the last element of the list
 *
 * @param[in] l list
 * @pre l != NULL
 *
 * @pre list size > 0
 * @return Value before deletion
 **/
void *genListPop(GenList *l);

/**
 * @brief Removes the element at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i < listSize
 * @return Value before deletion
 */
void *genListRemove(GenList *l, unsigned i);

/**
 * @brief Reads the value at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i Position of the element
 *
 * @pre l != NULL
 * @pre i < list size
 *
 * @return Read value
 **/
void *genListGet(GenList *l, unsigned i);

/**
 * @author VALLAT Ugo
 * @brief Changes the value at position i to a new value
 *
 * @param[in] l Pointer to the list
 * @param[in] v New value
 * @param[in] i Position
 * @pre l != NULL
 */
void genListSet(GenList *l, void *v, unsigned i);

/**
 *
 * @brief Returns if the list is empty
 *
 * @param[in] l Pointer to the list
 * @pre l != NULL
 *
 * @return true if empty, false otherwise
 */
bool genListIsEmpty(GenList *l);

/**
 * @author LAFORGE Mateo
 * @brief Searches for an element e in a list l and returns a corresponding boolean
 *
 * @param[in] l the list in which to search
 * @param[in] e the element to search for
 *
 * @return true if e is in l, false otherwise
 */
bool genListContains(GenList *l, void *e);

/**
 * @brief Returns the size of the list (position + 1 of the last element)
 *
 * @param[in] l Pointer to the list
 * @pre l != NULL
 *
 * @return size of the list
 *
 */

unsigned genListSize(GenList *l);

/**
 * @brief Copies the input list
 *
 * @param[in] l Pointer of the list to copy
 * @pre l != NULL
 *
 * @return  Pointer to the copy
 *
 */
GenList *genListCopy(GenList *l);

/**
 * @brief Return a description of the element (toString)
 *
 */
typedef char *(*printGen)(void *);

/**
 * @brief Display elements of genList in the logger
 *
 * @param l List to display
 * @param fun Function to display elements
 */
void genListPrintl(GenList *l, printGen fun);

#endif