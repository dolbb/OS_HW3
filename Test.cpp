
#include "hashtable.h"
#include "stdlib.h"
#include <assert.h>
#include "test_aux.h"
#include <stdio.h>
#include <map>

#define ELEMENT_NUMBER 10
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

const int iBuckets = 10;
#define NUM_OF_VARS 10000

#define PREPARE_TEST_VARIABLES()									\
		hashtable_t* hashTable = hash_alloc(iBuckets, Hasher);		\
		assert(hashTable);											\
		int vars[NUM_OF_VARS];										\
		for (int i = 0; i < NUM_OF_VARS; ++i)						\
		{															\
			vars[i] = rand();										\
		}

#define INSERT_VARIABLES_TO_HASH()									\
		for (int i = 0; i < NUM_OF_VARS; ++i)						\
		{															\
			assert(hash_insert(hashTable, i, vars + i) == 1);		\
		}

#define VERIFY_ALL_VARIABLES_EXIST_IN_HASH()						\
		for (int i = 0; i < NUM_OF_VARS; ++i)						\
		{															\
			assert(hash_contains(hashTable, i) == 1);				\
			assert(hash_insert(hashTable, i, vars + i) == 0);		\
		}
		

int Hasher(int numOfBuckets, int iKeyToBeHashed)
{
	return iKeyToBeHashed % numOfBuckets;
}

void* compute_func (void* a)
{
	return a;
}

int test_hash_insert() {
	PREPARE_TEST_VARIABLES();
	INSERT_VARIABLES_TO_HASH();
	VERIFY_ALL_VARIABLES_EXIST_IN_HASH();

	hash_stop(hashTable);
	hash_free(hashTable);

	return 1;
}

int test_hash_update() {
	PREPARE_TEST_VARIABLES();
	INSERT_VARIABLES_TO_HASH();

	for (int i = 0; i < NUM_OF_VARS; ++i)						
	{
		vars[i] = rand();
		assert(hash_update(hashTable, i, vars + i) == 1);		
	}
	VERIFY_ALL_VARIABLES_EXIST_IN_HASH();

	hash_stop(hashTable);
	hash_free(hashTable);

	return 1;
}

int test_hash_remove() {
	PREPARE_TEST_VARIABLES();
	INSERT_VARIABLES_TO_HASH();

	for (int i = 0; i < NUM_OF_VARS; ++i)
	{
		assert(hash_remove(hashTable, i) == 1);
		assert(hash_remove(hashTable, i) == 0);
		assert(hash_contains(hashTable, i) == 0);
		assert(hash_insert(hashTable, i, vars + i) == 1);
	}

	assert(hash_free(hashTable) == 0);
	assert(hash_stop(hashTable) == 1);
	assert(hash_free(hashTable) == 1);

	return 1;
}

void peformRandomOperation(hashtable_t* hashTable, int* buckets, std::map<int, int>& Map)
{
	// Get a random number
	int randomNum = rand() % 5; // 5 = number of operations testes
	
	// Convert number to operation
	switch (randomNum)
	{
	case 0: // Insert
		// Verify not exceeding size of buckets array
		if (Map.size() < NUM_OF_VARS)
		{
			// Get random key and value
			int key = rand();
			int val = rand();
			
			// Key is already in
			if (Map.find(key) != Map.end())
			{
				printf("DEBUG: (%d,%d) is already in.\n", key, val);

				// Verify already in
				assert(hash_insert(hashTable, key, &val) == 0);
				assert(hash_contains(hashTable, key) == 1);
			}

			// Key is not in
			else
			{
				// Insert
				Map[key] = val;
				assert(hash_contains(hashTable, key) == 0);
				assert(hash_insert(hashTable, key, &Map[key]) == 1);
				assert(hash_contains(hashTable, key) == 1);
				buckets[Hasher(iBuckets, key)]++;
			}

			int* pValInHash = NULL;
			assert(list_node_compute(hashTable, key, compute_func, (void**)(&pValInHash)) == 1);
			assert(*pValInHash == val);
		} 
		else
			printf("buckets Array is full.\n");
		break;

	case 1: // Update
		// int hash_update(hashtable_t* table, int key, void *val);
		if (Map.size() < NUM_OF_VARS)
		{
			// Get random key and value
			int key = rand();
			int val = rand();

			// If key is already in
			if (Map.find(key) != Map.end())
			{
				assert(hash_contains(hashTable, key) == 1);
				assert(hash_insert(hashTable, key, &val) == 0);
				assert(hash_contains(hashTable, key) == 1);

				int iValInHash;
				int* pValInHash = &iValInHash;
				assert(list_node_compute(hashTable, key, compute_func, (void**)(&pValInHash)) == 1);
				assert(iValInHash == val);
			}

			// If key is not in
			else
			{
				assert(hash_contains(hashTable, key) == 0);
				int tmpVal = rand();
				assert(hash_update(hashTable, key, &tmpVal) == 0);
				assert(hash_contains(hashTable, key) == 0);
			}
		}
		break;

	// Remove
	case 2:	
		// Check if there is an element to remove
		if (Map.size() != 0)
		{
			// Get random index of key in map
			int iKeyIndex = rand() % Map.size();
			
			// Find the key of iKeyIndex
			std::map<int,int>::iterator it = Map.begin();
			for (int i=0 ; i<iKeyIndex ; ++i)
				++it;
			
			int iKey = it->first;
			
			// Remove
			Map.erase(iKey);
			assert(hash_remove(hashTable, iKey) == 1);
			buckets[Hasher(iBuckets, iKey)]--;
		}
		
		// Hash is empty
		else
		{
			// Verify can't remove value from empty hash
			assert(hash_remove(hashTable, rand()) == 0);
			
			// Verify can't update non existing value
			int iTmpVal = rand();
			assert(hash_update(hashTable, rand(), (void*)(&iTmpVal)) == 0);
			
			// Verify all buckets are empty
			for (int i=0 ; i<iBuckets ; ++i)
				assert(hash_getbucketsize(hashTable, i) == 0);
		}
		break;

	// hash_contains
	case 3:
		// Check if there is an element to remove
		if (Map.size() != 0)
		{
			// Get random index of key in map
			int iKeyIndex = rand() % Map.size();
		
			// Find the key of iKeyIndex
			std::map<int,int>::iterator it = Map.begin();
			for (int i=0 ; i<iKeyIndex ; ++i)
				++it;
		
			int iKey = it->first;
		
			assert(hash_contains(hashTable, iKey) == 1);
			
		}
		
		// Hash is empty
		else
		{
			int iTmpVal = rand();
			assert(hash_contains(hashTable, rand()) == 0);
		}
		break;

	// hash_getbucketsize
	case 4:
		for (int i=0 ; i<iBuckets ; ++i)
			assert(hash_getbucketsize(hashTable, i) == buckets[i]);
		break;

	default:
		assert(0);

	}
}

int test_hash_random_operation()
{
	hashtable_t* hashTable = hash_alloc(iBuckets, Hasher);	
	int buckets[iBuckets] = { 0 };	// buckets[i] = hash_getbucketsize[i]
	std::map<int, int> Map;

	for (int i=0 ; i< 10000000 ; ++i)
	{
		peformRandomOperation(hashTable, buckets, Map);
	}
	
	return 1;
}

int main()
{
	TEST(test_hash_insert);
	TEST(test_hash_update);
	TEST(test_hash_remove);
	
	TEST(test_hash_random_operation);
	printf("\n%-10s***********All tests have passed***********%s\n", KGRN, KWHT);

	return 0;
}