/*================================================
*					includes
*===============================================*/
#include "OS_list.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

/*================================================
*					macros
*===============================================*/
#define CHECK_NULL_AND_RETURN_INVALID(ptr)		\
	if(ptr == NULL){							\
		return -1;								\
	}

#define CHECK_NULL_AND_RETURN_NULL(ptr)			\
	if(ptr == NULL){							\
		return NULL;							\
	}

#define NULL_PROVE_CHECK						\
	CHECK_NULL_AND_RETURN_INVALID(listHead)		\
	CHECK_NULL_AND_RETURN_INVALID(val)					
	//TODO: shall we allow "val == NULL"?

#define INIT_KEY -1

/*================================================
*					defines
*===============================================*/
struct node_t{
   struct node_t* next;
   int key;
   Element element;
   // pthread_mutex_t listMutex;
};

typedef struct node_t *Node;

/*================================================
*					functions
*===============================================*/
Node node_create(int key, Element val){
	assert(val);
	Node newNode = (Node)list_create();
	CHECK_NULL_AND_RETURN_NULL(newNode)
	newNode->key = key;
	newNode->element = val;
	return newNode;
}

List list_create(){
	//allocate:
	Node newList = malloc(sizeof(struct node_t));
	CHECK_NULL_AND_RETURN_NULL(newList)
	
	//init fields:
	newList->next = NULL;
	newList->key = INIT_KEY;
	newList->element = NULL;

	//init lock:
	// pthread_mutexattr_t att;
	// int pthread_mutexattr_init(&att);
	// pthread_mutex_init(&(newList->listMutex),&att);

	return (List)newList;
}

static void node_destroy(Node node){
	if(node){
		//TODO: delete lock. (release first if needed)
		free(node);
	}
}

void list_destroy(List listHeadL){
	Node listHead = (Node)listHeadL;
	Node tmpNode;
	while (listHead){
		tmpNode = listHead;
		listHead = listHead->next;
		node_destroy(tmpNode);
	}
}

//	locks the previous (node) to the node contains key.
//	return: pointer to previous node after it is locked.
static Node get_prev_node_to_element(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	assert(listHead);
	//TODO: lock listHead.
	Node prev = listHead; 
	Node curr = listHead->next;
	while(curr){
		//TODO: lock curr
		if(curr->key == key){
			//TODO: unlock curr
			return prev;
		}
		//TODO: release prev
		prev = curr;
		curr = prev->next;
	}
	return prev;
}

int list_insert(List listHeadL, int key, Element val){
	Node listHead = (Node)listHeadL;
	NULL_PROVE_CHECK

	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev != NULL);
	
	//if prev is NOT last - the element was found.
	if(prev->next){
		//TODO: release prev lock.
		return 0;
	}

	Node newNode = node_create(key,val);
	if(!newNode){
		//TODO: release prev lock.
		return -1;
	}
	newNode->next = prev->next;
	prev->next = newNode;
	//TODO free prev lock.
	return 1;
}

int list_update(List listHeadL, int key, Element val){
	Node listHead = (Node)listHeadL;
	NULL_PROVE_CHECK

	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev);
	
	//if prev is last - element wasnt found.
	if(!prev->next){
		//TODO: release prev lock.
		return 0;
	}
	Node curr = prev->next;
	assert(curr);
	//TODO: lock curr.
	//TODO: release prev.
	curr->element = val;
	//TODO: release cuur.
	return 1;
}

int list_remove(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_INVALID(listHead)

	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev);
	
	//if prev is last - element wasnt found.
	if(!prev->next){
		//TODO: release prev lock.
		return 0;
	}
	Node curr = prev->next;
	assert(curr);
	//TODO: lock curr.
	prev->next = curr->next;
	//TODO: release prev.
	node_destroy(curr);
	return 1;	
}

int list_contains(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_INVALID(listHead)

	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev);

	//if prev is last - element wasnt found.
	if(!prev->next){
		//TODO: release prev lock.
		return 0;
	}

	Node curr = prev->next;
	int res = -1;
	assert(curr);
	//TODO: lock curr.
	//TODO: release prev.
	assert(curr->key == key);
	if(curr->key == key){
		res = 1;
	}
	//TODO: release curr.
	return res;
}

int list_size(List listHeadL){
	Node listHead = (Node)listHeadL;
	//TODO: lock mutex
	CHECK_NULL_AND_RETURN_INVALID(listHead)

	int counter = -1;
	do{
		counter++;
		listHead = listHead->next;
	}while(listHead);
	
	//TODO: release lock
	return counter;
}

Element get_element_from_list(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_NULL(listHead)
	listHead = get_prev_node_to_element(listHead,key);
	CHECK_NULL_AND_RETURN_NULL(listHead->next)
	return listHead->next->element;
}