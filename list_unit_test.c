#include "OS_list.h"
#include "test_aux.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define ELEMENT_NUMBER 900
#define AMPLIFIER 100
#define LIST_ILLEGAL_PARAM -1
#define LIST_SUCCESSFUL_INSERT 1
#define LIST_ALREADY_EXISTS 0
#define LIST_SUCCESSFUL_REMOVE 1
#define LIST_DOESNT_EXIST 0
#define LIST_ELEMENT_EXISTS 1
#define NO_ELEMENTS_IN_LIST 0
#define LIST_SUCCESSFUL_UPDATE 1
#define LIST_SUCCESSFUL_COMPUTE 1

void *tmp_compute(void *element){
	int *n = (int*)malloc(sizeof(int));
	*n = *(int*)element + 1;
	return (void*)n;
}

int test_list_create_and_destroy(){
	List list1 = list_create();
	assert(list1);
	List list2 = list_create();
	assert(list2);
	List list3 = list_create();
	assert(list3);

	list_destroy(list1);
	list_destroy(list2);
	list_destroy(list3);

	return 1;
}

int test_list_insert(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	//insert all elements to list:
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		assert(list_insert(newList, keys[i], elements[i]) == LIST_SUCCESSFUL_INSERT);
		assert(list_insert(newList, keys[i], elements[i]) == LIST_ALREADY_EXISTS);	
		assert(*((int*)get_element_from_list(newList,keys[i])) == numbers[i]);
	}
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_insert(newList, keys[i], elements[i]) == LIST_ALREADY_EXISTS);	
	}

	list_destroy(newList);
	return 1;
}

int test_list_size(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	
	assert(list_size(NULL) == LIST_ILLEGAL_PARAM);
	assert(list_size(newList) == 0);

	//insert all elements to list:
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		list_insert(newList, keys[i], elements[i]);
		assert(list_size(newList) == i + 1);
	}

	list_destroy(newList);
	return 1;
}

int test_list_remove(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	
	assert(list_remove(NULL,0) == LIST_ILLEGAL_PARAM);
	//insert all elements to list:
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		list_insert(newList, keys[i], elements[i]);
	}

	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_remove(newList, keys[i]) == LIST_SUCCESSFUL_REMOVE);
		assert(list_remove(newList, keys[i]) == LIST_DOESNT_EXIST);	
		assert(list_size(newList) == ELEMENT_NUMBER - i - 1);
	}

	list_destroy(newList);
	return 1;
}

int test_list_contains(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	
	assert(list_contains(NULL,0) == LIST_ILLEGAL_PARAM);

	//insert all elements to list:
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		list_insert(newList, keys[i], elements[i]);
		assert(list_contains(newList,keys[i]) == LIST_ELEMENT_EXISTS);
	}
	
	//remove all elements
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		list_remove(newList, keys[i]);
		assert(list_contains(newList,keys[i]) == LIST_DOESNT_EXIST);
		assert(list_size(newList) == ELEMENT_NUMBER - i - 1);
	}
	
	//validate all is gone
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_contains(newList,keys[i]) == LIST_DOESNT_EXIST);
	}

	//reinsert all elements
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_size(newList) == i);
		list_insert(newList, keys[i], elements[i]);
		assert(list_size(newList) == i+1);
	}

	//validate all is here again
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_contains(newList,keys[i]) == LIST_ELEMENT_EXISTS);
	}

	list_destroy(newList);
	return 1;
}

int test_list_update(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	
	int num0 = 0;
	Element element0 = &num0;

	assert(list_update(NULL,0,element0) == LIST_ILLEGAL_PARAM);
	//insert all elements to list
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		list_insert(newList, keys[i], elements[i]);
	}

	//update all elements to 0
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_update(newList,keys[i],element0) == LIST_SUCCESSFUL_UPDATE);
		assert(*((int*)get_element_from_list(newList,keys[i])) == 0);
	}

	//check if elements are still there
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_contains(newList,keys[i]) == LIST_ELEMENT_EXISTS);
	}

	//update all elements back to ele[i]
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(list_update(newList,keys[i],elements[i]) == LIST_SUCCESSFUL_UPDATE);
		assert(*((int*)get_element_from_list(newList,keys[i])) == numbers[i]);
	}

	list_destroy(newList);
	return 1;
}

int test_compute_node(){
	List newList = list_create();
	
	int keys[ELEMENT_NUMBER];
	int numbers[ELEMENT_NUMBER];
	Element elements[ELEMENT_NUMBER];
	
	int num0 = 0;
	Element element0 = &num0;

	assert(list_update(NULL,0,element0) == LIST_ILLEGAL_PARAM);
	//insert all elements to list
	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		keys[i] = i * AMPLIFIER;
		numbers[i] = i;
		elements[i] = &numbers[i];
		list_insert(newList, keys[i], elements[i]);
	}

	int* Ptmp = NULL;
	assert(compute_node(NULL,-1, NULL, NULL) == LIST_ILLEGAL_PARAM);
	assert(compute_node(NULL, 0, tmp_compute, NULL) == LIST_ILLEGAL_PARAM);
	assert(compute_node(NULL, 0, tmp_compute, (void**)&Ptmp) == LIST_ILLEGAL_PARAM);
	assert(compute_node(newList, 0, tmp_compute, NULL) == LIST_ILLEGAL_PARAM);
	assert(Ptmp == NULL);

	for(int i = 0 ; i<ELEMENT_NUMBER ; ++i){
		assert(compute_node(newList, keys[i], tmp_compute, (void**)&Ptmp) == LIST_SUCCESSFUL_COMPUTE);		
		assert(*Ptmp == numbers[i] + 1);
		assert(get_element_from_list(newList, keys[i]) == elements[i]);
		free(Ptmp);
	}

	list_destroy(newList);
	return 1;
}

int main(){
	//tests without thread checks:
	TEST(test_list_create_and_destroy)
	TEST(test_list_insert)
	TEST(test_list_size)
	TEST(test_list_remove)
	TEST(test_list_contains)
	TEST(test_list_update)
	TEST(test_compute_node)
	printf("\n%-10s***********All tests have passed***********%s\n",KGRN,KWHT);
	return 0;
}
