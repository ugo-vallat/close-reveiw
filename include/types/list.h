/**
 * @file list.h
 * @author VALLAT Ugo, translation from french to english by MARTIN Benoit
 *
 * @brief This library implements a pseudo static list of integers
 *
 * Implements the list in the form of a static array and allocates
 * dynamic memory when it is full
 *
 * @note high performance in reading (O(1)) but low in writing (O(n))
 *
 * @remark In case of error, all list functions exit the program with an
 * error message
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#define LIST_TYPE pthread_t

/*------------------------------------------------------------------*/
/*                        LIST STRUCTURE                            */
/*------------------------------------------------------------------*/

/* Opaque definition of the list structure */
typedef struct s_list List;
typedef List *ptrList;

/**
 * @date  5/11/2023
 * @brief Creates an empty list
 *
 * @param[in] memory_size Initial memory space (in number of elements)
 *
 * @return pointer to the list
 * @note Allocates memory but is not initialized (list size = 0)
 */
List *initList(unsigned memory_size);

/**
 * @date  5/11/2023
 * @brief Deletes the list and frees the memory
 *
 * @param[in] l list to delete
 * @pre l != NULL
 * @pre *l != NULL
 */
void deinitList(ptrList *l);

/**
 * @date  5/11/2023
 * @brief Adds the element to the end of the list
 *
 * @param[in] l Pointer to the list
 * @param[in] v Value to add
 * @pre l != NULL
 */
void listAdd(List *l, LIST_TYPE v);

/**
 * @date  5/11/2023
 * @brief Inserts a value at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] v Value to add
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i <= listSize
 */
void listInsert(List *l, LIST_TYPE v, unsigned i);

/**
 * @date 5/11/2023
 * @brief Removes the last element of the list
 *
 * @param[in] l list
 * @pre l != NULL
 *
 * @pre list size > 0
 * @return value before deletion
 **/
LIST_TYPE listPop(List *l);

/**
 * @date  5/11/2023
 * @brief Removes the element at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i position
 * @pre l != NULL
 *
 * @pre i < listSize
 * @return value before deletion
 */
LIST_TYPE listRemove(List *l, unsigned i);

/**
 * @date 5/11/2023
 * @brief Reads the value at position i
 *
 * @param[in] l Pointer to the list
 * @param[in] i Position of the element
 * @pre l != NULL
 *
 * @pre i < list size
 *
 * @return Read value
 **/
LIST_TYPE listGet(List *l, unsigned i);

/**
 * @author VALLAT Ugo
 * @date 31/10/2023
 * @brief Changes the value at position i to a new value
 *
 * @param[in] l Pointer to the list
 * @param[in] v New value
 * @param[in] i Position
 * @pre l != NULL
 */
void listSet(List *l, LIST_TYPE v, unsigned i);

/**
 * @date 5/11/2023
 *
 * @brief Returns if the list is empty
 *
 * @param[in] l Pointer to the list
 * @pre l != NULL
 * @return true if empty, false otherwise
 */
bool listIsEmpty(List *l);

/**
 * @date  5/11/2023
 * @brief Returns the size of the list (position + 1 of the last element)
 *
 * @param[in] l Pointer to the list
 * @return size of the list
 *
 */

unsigned listSize(List *l);

/**
 * @date 30/10/2023
 * @brief Copies the input list
 *
 * @param[in] l Pointer of the list to copy
 * @pre l != NULL
 *
 * @return  Pointer to the copy
 *
 */

List *listCopy(List *l);

/**
 * @brief Resets the list to 0
 *
 * @param l List to empty
 */
void listClear(List *l);

#endif

