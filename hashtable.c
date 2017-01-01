
#include "assert.h"
#include "hashtable.h"
#include "OS_list.h"
#include "stdlib.h"
#include "stdbool.h"
#include <pthread.h>
#include <string.h>
#include "stdio.h"

struct hashtable_t
{
	int(*m_pHash)(int, int); // (number of buckets, key to be hashed)
	List* m_arrLists;
	int m_iBuckets;

	bool m_bStop;
	pthread_mutex_t stopMutex;

	unsigned int m_uiThreadsCounter;
	pthread_mutex_t counterMutex;

};

typedef struct do_hash_op_param_t
{
	op_t* op_data;
	hashtable_t* pHashTable;
} *do_hash_op_param_ptr;

#define DO_OP_BUCKET(LIST_OPERATION, BUCKET)						\
	do {															\
		int res = AtomicAddOperation(pHashTable);					\
		if (res == 1)												\
		{															\
			if (BUCKET < 0 || BUCKET >= pHashTable->m_iBuckets)		\
				res = -1;											\
			else													\
			{														\
				List listHead = pHashTable->m_arrLists[BUCKET];		\
				res = LIST_OPERATION;								\
			}														\
			AtomicAddToThreadsCounter(pHashTable, -1);				\
		}															\
		return res;													\
	} while(0)

#define DO_OP(LIST_OPERATION)															\
	do {																				\
		DO_OP_BUCKET(LIST_OPERATION, pHashTable->m_pHash(pHashTable->m_iBuckets, key)); \
	} while(0)

//////////////////////
//	Atomic Helpers	//
//////////////////////

static bool AtomicTestStopped(hashtable_t* pHashTable)
{
	bool bStopped = false;

	// Acquire stop lock
	pthread_mutex_lock(&(pHashTable->stopMutex));

	// Verify if already stopped
	if (pHashTable->m_bStop)
		bStopped = true;

	// Release stop lock
	pthread_mutex_unlock(&(pHashTable->stopMutex));

	return bStopped;
}

// Sets "stop = true" and returns it's previous value
static bool AtomicTestAndSetStopped(hashtable_t* pHashTable)
{
	bool bAlreadyStopped = false;
		
	// Acquire stop lock
	pthread_mutex_lock(&(pHashTable->stopMutex));

	// Verify if already stopped
	if (pHashTable->m_bStop)
		bAlreadyStopped = true;
	else
		pHashTable->m_bStop = true;

	// Release stop lock
	pthread_mutex_unlock(&(pHashTable->stopMutex));

	return bAlreadyStopped;
}

static unsigned int AtomicTestThreadsCounter(hashtable_t* pHashTable)
{
	// Acquire counter lock
	pthread_mutex_lock(&(pHashTable->counterMutex));

	unsigned int uiThreadsCounter = pHashTable->m_uiThreadsCounter;

	// Release counter lock
	pthread_mutex_unlock(&(pHashTable->counterMutex));

	return uiThreadsCounter;
}

static void AtomicAddToThreadsCounter(hashtable_t* pHashTable, int iDelta)
{
	// Acquire counter lock
	pthread_mutex_lock(&(pHashTable->counterMutex));

	// Increase threads counter
	pHashTable->m_uiThreadsCounter += iDelta;

	// Release counter lock
	pthread_mutex_unlock(&(pHashTable->counterMutex));
}

/*
Test stop and inc threadsCounter (if possible)
Returns:	1 if succeeded
			-1 if stopped
*/
static int AtomicAddOperation(hashtable_t* pHashTable)
{
	int iRes = -1;

	// Acquire counter lock
	pthread_mutex_lock(&(pHashTable->stopMutex));

	// Verify hash table didn't stop
	if (pHashTable->m_bStop == false)
	{
		// Increase Threads counter by 1
		AtomicAddToThreadsCounter(pHashTable, 1);

		// Update result
		iRes = 1;
	}

	// Release stop lock
	pthread_mutex_unlock(&(pHashTable->stopMutex));

	return iRes;
}

//////////////////////////////////
//	hashtable implementation	//
//////////////////////////////////

/*
Purpose:	Allocates and initializes a new hash table
Parameters:	iBuckets – the number of buckets in the hash table.
			pHash – the hash function the table will use.
Returns:	Pointer to table (or NULL in case of failure)
*/
hashtable_t* hash_alloc(int iBuckets, int(*pHash)(int, int))
{
	hashtable_t* pHashTable = NULL;

	// Verify input
	if (iBuckets <= 0 || pHash == NULL)
		return NULL;

	// Allocate table
	pHashTable = (hashtable_t*)malloc(sizeof(*pHashTable));

	// Verify table allocation
	if (!pHashTable)
		return NULL;

	// Allocate buckets array
	pHashTable->m_arrLists = (List*)malloc(iBuckets * sizeof(*(pHashTable->m_arrLists)));

	// Verify buckets array allocation
	if (!pHashTable->m_arrLists)
	{
		free(pHashTable);
		return NULL;
	}

	// Allocate every bucket
	for (int iCurrentBucket = 0; iCurrentBucket < iBuckets; ++iCurrentBucket)
	{
		pHashTable->m_arrLists[iCurrentBucket] = list_create();

		// Verify current bucket allocation
		if (pHashTable->m_arrLists[iCurrentBucket] == NULL)
		{
			// Deallocate previous buckets
			for (int i = 0; i < iCurrentBucket; ++i)
				list_destroy(pHashTable->m_arrLists[i]);

			// Deallocate previous resources
			free(pHashTable->m_arrLists);
			free(pHashTable);

			return NULL;
		}
	}

	// Initialize basic hashTable members
	pHashTable->m_pHash = pHash;
	pHashTable->m_iBuckets = iBuckets;
	pHashTable->m_bStop = false;
	pHashTable->m_uiThreadsCounter = 0;
	
	// Initialize stopMutex and counterMutex
	pthread_mutex_init(&(pHashTable->stopMutex),NULL);
	pthread_mutex_init(&(pHashTable->counterMutex),NULL);

	return pHashTable;
}

/*
Purpose:	Prepares hash table for deallocation.
Parameters:	pHashTable – pointer to the hash table
Returns:	1: The table was stopped
			-1: Error
*/
int hash_stop(hashtable_t* pHashTable)
{
	int iRes = 1;

	// Verify input
	if (!pHashTable)
		iRes = -1;

	// Verify not already stopped
	else if (AtomicTestAndSetStopped(pHashTable) == true)
		iRes = -1;

	// Wait for processes to finish
	else
	{
		while (AtomicTestThreadsCounter(pHashTable) != 0)
			sched_yield();
	}

	return iRes;
}

/*
Purpose:	Frees a previously allocated hash table.
Parameters:	pHashTable – pointer to the hash table
Returns:	1: The table was freed
			0: The table wasn’t stopped (hash_stop)
			-1: Error
*/
int hash_free(hashtable_t* pHashTable)
{
	// Assuming hash_free will be called once (according to "free after free" in piazza)
	int iRes = 1;

	// Verify input
	if (!pHashTable)
		iRes = -1;

	// Verify stopped
	else if (AtomicTestStopped(pHashTable) == false)
		iRes = 0;

	// Deallocate resources
	else
	{
		// Deallocate mutexes
		pthread_mutex_destroy(&(pHashTable->stopMutex));
		pthread_mutex_destroy(&(pHashTable->counterMutex));
		
		// Deallocate every bucket
		for (int iCurrentBucket = 0; iCurrentBucket < pHashTable->m_iBuckets; ++iCurrentBucket)
			list_destroy(pHashTable->m_arrLists[iCurrentBucket]);

		// Deallocate array
		free(pHashTable->m_arrLists);

		// Deallocate hash table
		free(pHashTable);
	}
	
	return iRes;
}

/*
This function inserts a single element with the value val into the hash table at key. Note that if
an element already exists in the hash table, it should not be added again.
Return values:
• 1: The element has been inserted
• 0: The element has not been inserted because it is already in the table
• -1: Error
*/
int hash_insert(hashtable_t* pHashTable, int key, void *val)
{
	// Verify input
	if (!pHashTable || !val)
		return -1;

	DO_OP(list_insert(listHead, key, val));
}

/*
This function update a single element with the key key the have the value .val.
Return values:
• 1: The element has been updated
• 0: The element has not been updated because it was not in the table
• -1: Error
*/
int hash_update(hashtable_t* pHashTable, int key, void *val)
{
	// Verify input
	if (!pHashTable || !val)
		return -1;

	DO_OP(list_update(listHead, key, val));
}

/*
This function removes a single element with the key key from the hash table.
Return values:
• 1: The element has been removed
• 0: The element has not been removed because it was not in the table
• -1: Error
*/
int hash_remove(hashtable_t* pHashTable, int key)
{
	// Verify input
	if (!pHashTable)
		return -1;

	DO_OP(list_remove(listHead, key));
}

/*
This function checks whether an element with the key key is in the hash table.
Return values:
• 1: The element is in the hash table
• 0: The element is not in the hash table
• -1: Error
*/
int hash_contains(hashtable_t* pHashTable, int key)
{
	// Verify input
	if (!pHashTable)
		return -1;

	DO_OP(list_contains(listHead, key));
}

/*
This function returns the number of elements currently in bucket number bucket, or -1 on error.
*/
int hash_getbucketsize(hashtable_t* pHashTable, int bucket)
{
	// Verify input
	if (!pHashTable)
		return -1;

	DO_OP_BUCKET(list_size(listHead), bucket);
}

/*
Find the node in the table according to the key and run user-provided function compute_func
on node's data. Put the result in the user-provided result.
Return values:
• 1: The function ran successfully
• 0: The element is not in the hash table
• -1: Error
*/
int list_node_compute(hashtable_t* pHashTable, int key, void *(*compute_func) (void *), void** result)
{
	// Verify input
	if (!pHashTable || !compute_func || !result)
		return -1;

	DO_OP(compute_node(listHead, key, compute_func, result));
}

void* do_hash_op(void* voidPParam)
{
	do_hash_op_param_ptr pParam = (do_hash_op_param_ptr)(voidPParam);
	
	switch(pParam->op_data->op)
	{
		case INSERT:
			pParam->op_data->result = hash_insert(pParam->pHashTable, pParam->op_data->key, pParam->op_data->val);
			break;
		
		case REMOVE:
			pParam->op_data->result = hash_remove(pParam->pHashTable, pParam->op_data->key);
			break;
			
		case CONTAINS:
			pParam->op_data->result = hash_contains(pParam->pHashTable, pParam->op_data->key);
			break;
			
		case UPDATE:
			pParam->op_data->result = hash_update(pParam->pHashTable, pParam->op_data->key, pParam->op_data->val);
			break;
			
		case COMPUTE:
			pParam->op_data->result = list_node_compute(pParam->pHashTable, pParam->op_data->key, pParam->op_data->compute_func, (void**)(&(pParam->op_data->val)));
			break;
		
		default:
			// Ignores current operation
			break;
	}
	
	// Deallocate pParam (allocateed in hash_batch)
	free(pParam);	
	pthread_exit(NULL);
	return NULL;
}

/*
Your hash table must support requests to perform several different operations concurrently. The
additional parameters passed to the hash_batch function are:
• num_ops – the number of operations in the batch
• ops - an array of pointers to op_t structures, where each op_t structure represents
a single-element operation.
*/
void hash_batch(hashtable_t* pHashTable, int num_ops, op_t* ops)
{
	// Verify input
	if (!pHashTable || num_ops<=0 || !ops)
		return;
	
	// Parameters for operations
	do_hash_op_param_ptr pCurrentParam;
	pthread_t arrThreads[num_ops];
	
	// Prepare parameters
	for(int i=0 ; i< num_ops ; ++i)
	{
		// Allocate current param
		pCurrentParam = (do_hash_op_param_ptr)malloc(sizeof(*pCurrentParam)); // Deallocated in do_hash_op
		
		// Verify allocation succeeded
		if(!pCurrentParam)
			continue;
		
		// Assign current param values
		pCurrentParam->op_data = ops+i;
		pCurrentParam->pHashTable = pHashTable;
		
		// Start current operation on a new thread
		pthread_create(arrThreads+i, NULL, do_hash_op, pCurrentParam);
	}
	
	// Wait for threads to finish
	for (int i=0 ; i<num_ops ; ++i)
		pthread_join(arrThreads[i], NULL);
}
