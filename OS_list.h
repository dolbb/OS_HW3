#ifndef OS_LIST_H
#define OS_LIST_H
/*================================================
*					includes
*===============================================*/
#include <stdlib.h>
#include <stdbool.h>

/*================================================
*					defines
*===============================================*/
typedef void *List;

typedef void *Element;

/*================================================
*					declerations
*===============================================*/

//	creates a new empty node to be a head of a list.
List list_create();

//	destroys a whole list after all threads have finished with it.
void list_destroy(List listHeadL);

//	insert a new node to the list, with the key and val specified.
//	return: 1 on success.
//	return: 0 if already exists.
//	return: -1 otherwise.
int list_insert(List listHeadL, int key, Element val);

//	finds and updates a node with the key and val specified.
//	return: 1 on success.
//	return: 0 if doesn't exist.
//	return: -1 otherwise.
int list_update(List listHeadL, int key, Element val);

//	removes a node from the list, with the key specified.
//	return: 1 on success.
//	return: 0 if doesn't exist.
//	return: -1 otherwise.
int list_remove(List listHeadL, int key);

//	checks if a node with the specified key exists.
//	return: 1 element exists.
//	return: 0 if doesn't exist.
//	return: -1 otherwise.
int list_contains(List listHeadL, int key);

//	iterate the list to count the number of elements\nodes in it.
//	return: non-negative = size of the list.
//	return: -1 otherwise (null pointer).
int list_size(List listHeadL);

//this function will compute the given func over the node with the key.
//changes the result ptr to the computing result.
//return: 1 - successful
//return: 0 - element not found
//return: 1 - error
int compute_node(List listHeadL, int key, void *(*compute_func) (void *), void** result);

//for tests ONLY:
Element get_element_from_list(List listHeadL, int key);

#endif