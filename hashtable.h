
#ifndef OS_HASHTABLE_H
#define OS_HASHTABLE_H 

struct hashtable_t;
typedef struct hashtable_t hashtable_t;

typedef struct op_t
{
	int key;
	void *val;
	enum { INSERT, REMOVE, CONTAINS, UPDATE, COMPUTE } op;
	void *(*compute_func) (void *);
	int result;
} op_t;

// Create + destroy hashtable
hashtable_t* hash_alloc(int buckets, int (*hash)(int, int));
int hash_stop(hashtable_t* table);
int hash_free(hashtable_t* table);

// Getters + setters
int hash_insert(hashtable_t* table, int key, void *val);
int hash_update(hashtable_t* table, int key, void *val);
int hash_remove(hashtable_t* table, int key);
int hash_contains(hashtable_t* table, int key);
int hash_getbucketsize(hashtable_t* table, int bucket);

// Operations on elements
int list_node_compute(hashtable_t* table, int key, void *(*compute_func) (void *), void** result);
void hash_batch(hashtable_t* table, int num_ops, op_t* ops);

#endif // OS_HASHTABLE_H