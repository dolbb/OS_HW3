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
   pthread_mutex_t listMutex;
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
	Node newList = (Node)malloc(sizeof(struct node_t));
	CHECK_NULL_AND_RETURN_NULL(newList)
	
	//init fields:
	newList->next = NULL;
	newList->key = INIT_KEY;
	newList->element = NULL;

	//init lock:
	pthread_mutexattr_t  attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr , PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&(newList->listMutex),&attr);
	pthread_mutexattr_destroy(&attr);
	return (List)newList;
}

static void node_destroy(Node node){
	//mutex inside node MUST be free here
	if(node){
		pthread_mutex_destroy(&(node->listMutex));
		free(node);
	}
}

void list_destroy(List listHeadL){
	Node listHead = (Node)listHeadL;
	//ALL mutexs inside the list MUST be free:
	Node tmpNode;
	while (listHead){
		tmpNode = listHead;
		listHead = listHead->next;
		node_destroy(tmpNode);
	}
}

//	locks the previous (node) to the node contains key.
//	return: pointer to previous node after it is locked.
static Node get_prev_node_to_element(Node listHead, int key){
	assert(listHead);
	//lock listHead:
	pthread_mutex_lock(&(listHead->listMutex));
	Node prev = listHead; 
	Node curr = listHead->next;
	while(curr){
		//lock curr:
		pthread_mutex_lock(&(curr->listMutex));
		if(curr->key == key){
			//unlock curr:
			pthread_mutex_unlock(&(curr->listMutex));
			return prev;
		}
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
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
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return 0;
	}

	Node newNode = node_create(key,val);
	if(!newNode){
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return -1;
	}
	newNode->next = prev->next;
	prev->next = newNode;
	//unlock prev:
	pthread_mutex_unlock(&(prev->listMutex));
	return 1;
}

int list_update(List listHeadL, int key, Element val){
	Node listHead = (Node)listHeadL;
	NULL_PROVE_CHECK

	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev);
	
	//if prev is last - element wasnt found.
	if(!prev->next){
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return 0;
	}
	Node curr = prev->next;
	assert(curr);
	//lock curr:
	pthread_mutex_lock(&(curr->listMutex));
	curr->element = val;
	//unlock prev:
	pthread_mutex_unlock(&(prev->listMutex));
	//unlock curr:
	pthread_mutex_unlock(&(curr->listMutex));
	return 1;
}

int list_remove(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_INVALID(listHead)

	Node prev = get_prev_node_to_element(listHead,key);
	assert(prev);
	
	//if prev is last - element wasnt found.
	if(!prev->next){
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return 0;
	}
	Node curr = prev->next;
	assert(curr);
	//lock curr:
	pthread_mutex_lock(&(curr->listMutex));
	prev->next = curr->next;
	//release prev:
	pthread_mutex_unlock(&(prev->listMutex));
	//release curr (wont be reachable here):
	pthread_mutex_unlock(&(curr->listMutex));
	node_destroy(curr);
	return 1;	
}

int list_contains(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_INVALID(listHead)

	Node prev = get_prev_node_to_element(listHead,key);
	assert(prev);

	//if prev is last - element wasnt found.
	if(!prev->next){
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return 0;
	}

	Node curr = prev->next;
	int res = -1;
	assert(curr);
	//lock curr:
	pthread_mutex_lock(&(curr->listMutex));
	assert(curr->key == key);
	if(curr->key == key){
		res = 1;
	}
	//unlock curr:
	pthread_mutex_unlock(&(curr->listMutex));
	//unlock prev:
	pthread_mutex_unlock(&(prev->listMutex));
	return res;
}

int list_size(List listHeadL){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_INVALID(listHead)
	assert(listHead);
	//lock listHead:
	pthread_mutex_lock(&(listHead->listMutex));

	Node prev = listHead; 
	Node curr = listHead->next; 
	
	int counter = 0;
	while(curr){
		curr = prev->next;
		if(curr){
			//lock curr:
			pthread_mutex_lock(&(curr->listMutex));
			//unlock prev:
			pthread_mutex_unlock(&(prev->listMutex));
			prev = curr;
			curr = prev->next;
		}
		++counter;
	}

	//unlock prev:
	pthread_mutex_unlock(&(prev->listMutex));
	return counter;
}

int compute_node(List listHeadL, int key, void *(*compute_func) (void *), void** result){
	CHECK_NULL_AND_RETURN_INVALID(listHeadL)
	CHECK_NULL_AND_RETURN_INVALID(compute_func)
	CHECK_NULL_AND_RETURN_INVALID(result)

	Node listHead = (Node)listHeadL;
	Node prev = get_prev_node_to_element(listHead,key);
	assert (prev != NULL);
	//if prev is last - element wasnt found.
	if(!prev->next){
		//unlock prev:
		pthread_mutex_unlock(&(prev->listMutex));
		return 0;
	}
	Node curr = prev->next;
	//lock curr:
	pthread_mutex_lock(&(curr->listMutex));
	//unlock prev:
	pthread_mutex_unlock(&(prev->listMutex));
	//compute the element and result:
	*result = compute_func(curr->element);
	//unlock curr:
	pthread_mutex_unlock(&(curr->listMutex));
	return 1; 
}

//======================================================
//				ONLY TO BE USED ON TESTS:
//======================================================
Element get_element_from_list(List listHeadL, int key){
	Node listHead = (Node)listHeadL;
	CHECK_NULL_AND_RETURN_NULL(listHead)
	listHead = get_prev_node_to_element(listHead,key);
	CHECK_NULL_AND_RETURN_NULL(listHead->next)
	return listHead->next->element;
}