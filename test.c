/*
 * test.c
 *
 *  Created on: Dec 14, 2016
 *      Author: Yair
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include "test_utilities.h"
#include "hashtable.h"


#define BUCKETS 10 //basic functionality test adds to hash according to this. changing will ruin outcome.
#define NUM_BUCKETS 50
#define STRESS_CMDS 1800
#define MAX_KEY 200
#define NUM_CMDS 500
#define NUM_LOOPS 20
#define CMDS 5
#define MAX_BUFSIZE 100
#define MAX_TIME 100

int hash_f(int buckets, int key) {
	return key % buckets;
}

void*  compute_f(void* val) {
	return val;
}

void* compute_f2(void* val){
	//saves in the same place!!!
	*(int*)val = (*(int*)val * 3);
	return val;
}

typedef void* (*comp_func)(void*);
typedef hashtable_t* hashtable;
int dispatch_log_idx=0;


//void randwait(pthread_mutex_t *mut, pthread_cond_t *cond) {
//	struct timespec t;
//	t.tv_sec = rand()%MAX_TIME;
//	t.tv_nsec = rand()%MAX_TIME * 1000;
//	pthread_mutex_lock(mut);
//	pthread_cond_timedwait(cond, mut, &t);
//	pthread_mutex_unlock(mut);
//}

/*
 * Wrapper functions so threads execute a hash command and then report result into pipe. hopefully
 * the pipe will not be used concurrently
 */
typedef struct insert_args_t {
	pthread_mutex_t *mut;
	hashtable h;
	int key;
	void* val;
	int w_pipe;
	int t_id;
}insert_args_t;

void* thread_insert(void *args) {
	insert_args_t* insert_args= (insert_args_t*)(args);
	pthread_t t_id = pthread_self();
	int w_pipe = insert_args->w_pipe;
	int res = hash_insert(insert_args->h, insert_args->key, insert_args->val);
	char ret[MAX_BUFSIZE] = {0};
	sprintf(ret,"Thread: %d returned with result: %d\n",insert_args->t_id, res);
	pthread_mutex_lock(insert_args->mut);
	write(w_pipe, ret, strlen(ret));
	pthread_mutex_unlock(insert_args->mut);
	free((insert_args_t*)args);
	return NULL;
}

typedef struct remove_args_t {
	pthread_mutex_t *mut;
	hashtable h;
	int key;
	int w_pipe;
	int t_id;
}remove_args_t;

void* thread_remove(void *args) {
	remove_args_t* remove_args= (remove_args_t*)(args);
	pthread_t t_id = pthread_self();
	int w_pipe = remove_args->w_pipe;
	int res = hash_remove(remove_args->h, remove_args->key);
	char ret[MAX_BUFSIZE] = {0};
	sprintf(ret, "Thread: %d returned with result: %d\n",remove_args->t_id, res);
	pthread_mutex_lock(remove_args->mut);
	write(w_pipe, ret, strlen(ret));
	pthread_mutex_unlock(remove_args->mut);
	free((remove_args_t*)args);
	return NULL;
}

typedef struct update_args_t {
	pthread_mutex_t *mut;
	hashtable h;
	int key;
	void* val;
	int w_pipe;
	int t_id;
}update_args_t;

void* thread_update(void *args) {
	update_args_t* update_args= (update_args_t*)(args);
	pthread_t t_id = pthread_self();
	int w_pipe = update_args->w_pipe;
	int res = hash_update(update_args->h, update_args->key, update_args->val);
	char ret[MAX_BUFSIZE] = {0};
	sprintf(ret, "Thread: %d returned with result: %d\n",update_args->t_id, res);
	pthread_mutex_lock(update_args->mut);
	write(w_pipe, ret, strlen(ret));
	pthread_mutex_unlock(update_args->mut);
	free((update_args_t*)args);
	return NULL;
}

typedef struct contains_args_t {
	pthread_mutex_t *mut;
	hashtable h;
	int key;
	int w_pipe;
	int t_id;
}contains_args_t;

void* thread_contains(void *args) {
	contains_args_t* contains_args= (contains_args_t*)(args);
	pthread_t t_id = pthread_self();
	int w_pipe = contains_args->w_pipe;
	int res = hash_contains(contains_args->h, contains_args->key);
	char ret[MAX_BUFSIZE] = {0};
	sprintf(ret, "Thread: %d returned with result: %d\n",contains_args->t_id, res);
	pthread_mutex_lock(contains_args->mut);
	write(w_pipe, ret, strlen(ret));
	pthread_mutex_unlock(contains_args->mut);
	free((contains_args_t*)args);
	return NULL;
}

typedef struct compute_args_t {
	pthread_mutex_t *mut;
	hashtable h;
	int key;
	comp_func f;
	int w_pipe;
	int t_id;
}compute_args_t;

void* thread_compute(void *args) {
	compute_args_t* compute_args= (compute_args_t*)(args);
	pthread_t t_id = pthread_self();
	int w_pipe = compute_args->w_pipe;
	void* comp_res;
	int res = list_node_compute(compute_args->h, compute_args->key, compute_args->f, &comp_res);
	char ret[MAX_BUFSIZE] = {0};
	sprintf(ret, "Thread: %d returned with result: %d\n",compute_args->t_id, res);
	pthread_mutex_lock(compute_args->mut);
	write(w_pipe, ret, strlen(ret));
	pthread_mutex_unlock(compute_args->mut);
	free((compute_args_t*)args);
	return NULL;
}

/*
 * Dispatches new threads to execute hash cmds. Never called by multiple threads.
 */
pthread_t thread_log_and_execute(hashtable h, op_t op, int w_pipe, pthread_mutex_t *mut){
	pthread_t t_id;
	char* filename = "hash_cmd.log";
	int f = open(filename, O_WRONLY | O_APPEND);
	char res_buff[MAX_BUFSIZE] = {0};
	if (f < 0) {
		printf("Error opening cmds log. errno: %d. Request canceled\n", errno);
		return 0;
	}
	dispatch_log_idx++;
	int res;
	if (op.op == INSERT){
        	insert_args_t* args = malloc(sizeof(insert_args_t));
        	args->mut = mut;
        	args->h = h;
        	args->key = op.key;
        	args->val = op.val;
        	args->w_pipe = w_pipe;
        	args->t_id = dispatch_log_idx;
		res = pthread_create(&t_id,NULL,thread_insert, (void*)args);
		sprintf(res_buff, "%s %d. %s %d. %s %d. %s %d\n","Record:", dispatch_log_idx, "Thread:", dispatch_log_idx, "executing: INSERT  . Key:", op.key, "thread_create result:",res);
		write(f, res_buff, strlen(res_buff));
	} else if (op.op == REMOVE) {
        	remove_args_t* args = malloc(sizeof(remove_args_t));
        	args->mut = mut;
        	args->h = h;
        	args->key = op.key;
        	args->w_pipe = w_pipe;
        	args->t_id = dispatch_log_idx;
		res = pthread_create(&t_id,NULL,thread_remove, (void*)args);
		sprintf(res_buff, "%s %d. %s %d. %s %d. %s %d\n","Record:", dispatch_log_idx, "Thread:", dispatch_log_idx, "executing: REMOVE  . Key:", op.key, "thread_create result:",res);
		write(f, res_buff, strlen(res_buff));
	} else if (op.op == UPDATE) {
        	update_args_t* args = malloc(sizeof(update_args_t));
        	args->mut = mut;
        	args->h = h;
        	args->key = op.key;
        	args->w_pipe = w_pipe;
        	args->t_id = dispatch_log_idx;
		res = pthread_create(&t_id,NULL,thread_update, (void*)args);
		sprintf(res_buff, "%s %d. %s %d. %s %d. %s %d\n","Record:", dispatch_log_idx, "Thread:", dispatch_log_idx, "executing: UPDATE  . Key:", op.key, "thread_create result:",res);
		write(f, res_buff, strlen(res_buff));
	} else if (op.op == CONTAINS){
        	contains_args_t* args = malloc(sizeof(contains_args_t));
        	args->mut = mut;
        	args->h = h;
        	args->key = op.key;
        	args->w_pipe = w_pipe;
        	args->t_id = dispatch_log_idx;
		res = pthread_create(&t_id,NULL,thread_contains, (void*)args);
		sprintf(res_buff, "%s %d. %s %d. %s %d. %s %d\n","Record:", dispatch_log_idx, "Thread:", dispatch_log_idx, "executing: CONTAINS. Key:", op.key, "thread_create result:",res);
		write(f, res_buff, strlen(res_buff));
	} else if (op.op == COMPUTE){
        	compute_args_t* args = malloc(sizeof(compute_args_t));
        	args->mut = mut;
        	args->h = h;
        	args->key = op.key;
        	args->f = op.compute_func;
        	args->w_pipe = w_pipe;
        	args->t_id = dispatch_log_idx;
		res = pthread_create(&t_id,NULL,thread_compute, (void*)args);
		sprintf(res_buff, "%s %d. %s %d. %s %d. %s %d\n","Record:", dispatch_log_idx, "Thread:", dispatch_log_idx, "executing: COMPUTE . Key:", op.key, "thread_create result:",res);
		write(f, res_buff, strlen(res_buff));
	}
	close(f);
	return t_id;
}



/* Basic functional test for hash table*/
int TestHashActions_Insert() {
	ASSERT_NULL(hash_alloc(0, hash_f));
	ASSERT_NULL(hash_alloc(-1, hash_f));
	ASSERT_NULL(hash_alloc(BUCKETS, NULL));
	hashtable_t *h = hash_alloc(BUCKETS, hash_f);
	ASSERT_NOT_NULL(h);

	int val1 = 1;
	int val2 = 2;
	int val3 = 3;
	int val4 = 4;

	ASSERT_EQ(hash_insert(NULL, 1, &val1), -1);
	ASSERT_EQ(hash_insert(NULL, -1, &val1), -1); //invalid key
	ASSERT_EQ(hash_insert(h, 1, &val1), 1);
	ASSERT_EQ(hash_insert(h, 1, &val1), 0);
	ASSERT_EQ(hash_insert(h, 1, &val2), 0);
	ASSERT_EQ(hash_insert(h, 2, &val2), 1);
	ASSERT_EQ(hash_insert(h, 2, &val2), 0);
	ASSERT_EQ(hash_insert(h, 12, &val2), 1);
	ASSERT_EQ(hash_insert(h, 3, &val3), 1);
	ASSERT_EQ(hash_insert(h, 4, &val4), 1);
	ASSERT_EQ(hash_stop(h), 1);
	ASSERT_EQ(hash_free(h), 1);
	return true;;
}

int TestHashActions_ContainsAndRemove() {
	hashtable_t *h = hash_alloc(BUCKETS, hash_f);
	ASSERT_NOT_NULL(h);

	int val1 = 1;
	int val2 = 2;
	int val3 = 3;
	int val4 = 4;

	ASSERT_EQ(hash_insert(h, 1, &val1), 1);
	ASSERT_EQ(hash_insert(h, 11, &val1), 1);
	ASSERT_EQ(hash_insert(h, 2, &val2), 1);
	ASSERT_EQ(hash_insert(h, 12, &val2), 1);
	ASSERT_EQ(hash_insert(h, 22, &val2), 1);
	ASSERT_EQ(hash_insert(h, 3, &val3), 1);
	ASSERT_EQ(hash_insert(h, 4, &val4), 1);

	ASSERT_EQ(hash_remove(NULL, 1), -1);
	//invalid key
	ASSERT_EQ(hash_remove(h, -1), -1);
	//element doesn't exist
	ASSERT_EQ(hash_remove(h, 5), 0);
	ASSERT_EQ(hash_remove(h, 13), 0);

	ASSERT_EQ(hash_contains(NULL, 1), -1);
	ASSERT_EQ(hash_contains(h, -1), -1);
	ASSERT_EQ(hash_contains(h, 5), 0);
	ASSERT_EQ(hash_contains(h, 13), 0);
	ASSERT_EQ(hash_contains(h, 1), 1);
	ASSERT_EQ(hash_contains(h, 11), 1);
	ASSERT_EQ(hash_contains(h, 2), 1);
	ASSERT_EQ(hash_contains(h, 12), 1);
	ASSERT_EQ(hash_contains(h, 3), 1);
	ASSERT_EQ(hash_contains(h, 4), 1);
	ASSERT_EQ(hash_remove(h, 1), 1);
	ASSERT_EQ(hash_remove(h, 1), 0);
	ASSERT_EQ(hash_contains(h, 1), 0);
	ASSERT_EQ(hash_contains(h, 11), 1);
	ASSERT_EQ(hash_remove(h, 11), 1);
	ASSERT_EQ(hash_remove(h, 11), 0);
	ASSERT_EQ(hash_contains(h, 11), 0);

	//removing from middle of bucket
	ASSERT_EQ(hash_contains(h, 22), 1);
	ASSERT_EQ(hash_remove(h, 12), 1);
	ASSERT_EQ(hash_contains(h, 22), 1);
	ASSERT_EQ(hash_contains(h, 12), 0);
	ASSERT_EQ(hash_contains(h, 2), 1);
	ASSERT_EQ(hash_remove(h, 22), 1);
	ASSERT_EQ(hash_remove(h, 2), 1);
	ASSERT_EQ(hash_contains(h, 22), 0);
	ASSERT_EQ(hash_contains(h, 12), 0);
	ASSERT_EQ(hash_contains(h, 2), 0);

	ASSERT_EQ(hash_stop(h), 1);
	ASSERT_EQ(hash_free(h), 1);
	return true;;
}

int TestHashActions_UpdateAndCompute() {
	hashtable_t *h = hash_alloc(BUCKETS, hash_f);
	ASSERT_NOT_NULL(h);

	int val1 = 1;
	int val2 = 2;
	int val3 = 3;
	int val4 = 4;
	//this is just to initialize res with something
	void* val4_ptr = (void*)(&val4);
	void **res = &val4_ptr;

	ASSERT_EQ(hash_insert(h, 1, &val1), 1);
	ASSERT_EQ(hash_insert(h, 2, &val2), 1);
	ASSERT_EQ(hash_insert(h, 12, &val2), 1);
	ASSERT_EQ(hash_insert(h, 3, &val3), 1);

	ASSERT_EQ(hash_update(NULL, 1, &val1), -1);
	ASSERT_EQ(hash_update(h, -5, &val1), -1);
	ASSERT_EQ(hash_update(h, 1, &val1), 1);
	ASSERT_EQ(hash_update(h, 1, &val2), 1);
	ASSERT_EQ(hash_remove(h, 1), 1);
	ASSERT_EQ(hash_update(h, 1, &val1), 0);
	ASSERT_EQ(hash_update(h, 12, &val2), 1);
	ASSERT_EQ(hash_update(h, 12, &val1), 1);

	ASSERT_EQ(hash_insert(h, 1, &val1), 1);
	ASSERT_EQ(list_node_compute(NULL, 1, compute_f, res), -1);
	ASSERT_EQ(list_node_compute(h, 1, NULL, res), -1);
	ASSERT_EQ(list_node_compute(h, 1, compute_f, NULL), -1);

	ASSERT_EQ(list_node_compute(h, 111, compute_f, res), 0);
	ASSERT_EQ(list_node_compute(h, 14, compute_f, res), 0);
	ASSERT_EQ(list_node_compute(h, 1, compute_f, res), 1);
	ASSERT_EQ(**(int**)(res), val1);
	ASSERT_EQ(list_node_compute(h, 2, compute_f, res), 1);
	ASSERT_EQ(**(int**)(res), val2);
	ASSERT_EQ(list_node_compute(h, 12, compute_f, res), 1);
	ASSERT_EQ(**(int**)(res), val1); //was updated to val1
	//lets update and verify compute is consistent
	ASSERT_EQ(hash_update(h, 1, &val4), 1);
	ASSERT_EQ(list_node_compute(h, 1, compute_f, res), 1);
	ASSERT_EQ(**(int**)(res), val4);
	ASSERT_EQ(**(int**)(res), 4);

	ASSERT_EQ(hash_stop(h), 1);
	ASSERT_EQ(hash_free(h), 1);
	return true;;
}


int TestHashActions_GetSizeAndStop() {
	hashtable_t *h = hash_alloc(BUCKETS, hash_f);
	ASSERT_NOT_NULL(h);

	int val1 = 1;
	int val2 = 2;
	int val3 = 3;
	int val4 = 4;

	ASSERT_EQ(hash_insert(h, 1, &val1), 1);
	ASSERT_EQ(hash_insert(h, 2, &val2), 1);
	ASSERT_EQ(hash_insert(h, 12, &val2), 1);
	ASSERT_EQ(hash_insert(h, 22, &val2), 1);
	ASSERT_EQ(hash_insert(h, 32, &val2), 1);
	ASSERT_EQ(hash_insert(h, 3, &val3), 1);
	ASSERT_EQ(hash_insert(h, 4, &val4), 1);
	ASSERT_EQ(hash_insert(h, 74, &val4), 1);

	ASSERT_EQ(hash_getbucketsize(NULL, 1), -1);
	ASSERT_EQ(hash_getbucketsize(h, -1), -1);
	ASSERT_EQ(hash_getbucketsize(h, BUCKETS), -1);

	ASSERT_EQ(hash_getbucketsize(h, 0), 0);
	ASSERT_EQ(hash_getbucketsize(h, 1), 1);
	ASSERT_EQ(hash_getbucketsize(h, 2), 4);
	ASSERT_EQ(hash_getbucketsize(h, 3), 1);
	ASSERT_EQ(hash_getbucketsize(h, 4), 2);

	for (int i = 5; i<BUCKETS; i++){
		ASSERT_EQ(hash_getbucketsize(h, i), 0);
	}

	ASSERT_EQ(hash_remove(h, 1), 1);
	ASSERT_EQ(hash_remove(h, 12), 1);
	ASSERT_EQ(hash_remove(h, 22), 1);

	ASSERT_EQ(hash_getbucketsize(h, 2), 2);
	ASSERT_EQ(hash_getbucketsize(h, 1), 0);
	ASSERT_EQ(hash_contains(h, 2), 1);
	ASSERT_EQ(hash_contains(h, 32), 1);
	ASSERT_EQ(hash_contains(h, 12), 0);
	ASSERT_EQ(hash_contains(h, 22), 0);

	ASSERT_EQ(hash_free(h), 0);
	ASSERT_EQ(hash_free(NULL), -1);
	ASSERT_EQ(hash_stop(NULL), -1);
	ASSERT_EQ(hash_stop(h), 1);
	ASSERT_EQ(hash_stop(h), -1);
	ASSERT_EQ(hash_free(h), 1);
	return true;
}

bool StressTestWithBatches() {
	int stress_num = STRESS_CMDS;
	int fifth=stress_num/5;
	int tenth=stress_num/10;
	op_t* ops_arr = malloc(sizeof(*ops_arr) * stress_num);
	ASSERT_NOT_NULL(ops_arr);
	//op_t **opts_ptrs = malloc(sizeof(*opts_ptrs)*stress_num);
	//ASSERT_NOT_NULL(opts_ptrs);
	int *vals = malloc(sizeof(*vals)*fifth);
	ASSERT_NOT_NULL(vals);
	int *vals_updated = malloc(sizeof(*vals_updated)*fifth);
	ASSERT_NOT_NULL(vals_updated);
	int *keys = malloc(sizeof(*keys)*fifth);
	ASSERT_NOT_NULL(keys);

	//Setting the keys and values array - values to be send as val pointers
	for(int i=0; i<fifth; i++){
		keys[i] = i;
		vals[i] = i;
		vals_updated[i] = i;
	}

	//Initializing the batch of commands
	for(int i=0; i<stress_num; i++){
		//set pointer of batch
		//opts_ptrs[i] = ops_arr + i;

		//set op_t
		ops_arr[i].key = (i%fifth);
		ops_arr[i].result = 0;
		ops_arr[i].compute_func = NULL;
		ops_arr[i].val = NULL;
		if (i < fifth){
			//insert
			ops_arr[i].val = vals + (i%fifth);
			ops_arr[i].op = INSERT;
		} else if (i < fifth * 2) {
			ops_arr[i].op = CONTAINS;
		} else if (i < fifth * 3) {
			ops_arr[i].op = COMPUTE;
			ops_arr[i].compute_func = compute_f2;
		} else if (i < fifth * 4) {
			ops_arr[i].op = UPDATE;
			ops_arr[i].val = vals_updated + (i%fifth);
		} else {
			ASSERT_LT(i, fifth*5);
			ops_arr[i].op = REMOVE;
		}
	}

	hashtable_t *h = hash_alloc(tenth, hash_f);
	ASSERT_NOT_NULL(h);

	//first fifth - adding items
	hash_batch(h, fifth, ops_arr);
	int sum=0;
	int curr=0;
	for(int i=0; i < tenth; i++){
		curr = hash_getbucketsize(h, i);
		ASSERT_EQ(curr, 2);
		sum += curr;
	}
	ASSERT_EQ(sum, fifth);
	for(int i=0; i < fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
	}

	//first fifth - adding items AGAIN - all should fail
	hash_batch(h, fifth, ops_arr);
	sum=0;
	for(int i=0; i < tenth; i++){
		curr = hash_getbucketsize(h, i);
		ASSERT_EQ(curr, 2);
		sum += curr;
	}
	ASSERT_EQ(sum, fifth);
	for(int i=0; i < fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,0);
	}

	//second fifth - check if contains
	hash_batch(h, fifth, ops_arr + (fifth));
	for(int i=fifth; i < 2*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
	}

	//third fifth - compute
	hash_batch(h, fifth, ops_arr + (fifth*2));
	for(int i=2*fifth; i < 3*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
		ASSERT_EQ(vals[i%fifth], 3*(i%fifth));//multplied by 3
	}

	//fourth fifth - update all
	hash_batch(h, fifth, ops_arr + (fifth*3));
	for(int i=3*fifth; i < 4*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
	}

	//third fifth AGAIN - compute
	hash_batch(h, fifth, ops_arr + (fifth*2));
	for(int i=2*fifth; i < 3*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
		ASSERT_EQ(vals_updated[i%fifth], 3*(i%fifth));//multplied by 3
	}

	//fifth fifth - remove
	hash_batch(h, fifth, ops_arr + (fifth*4));
	for(int i=4*fifth; i < 5*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,1);
	}
	sum=0;
	for(int i=0; i < tenth; i++){
		curr = hash_getbucketsize(h, i);
		ASSERT_EQ(curr, 0);
		sum += curr;
	}
	ASSERT_EQ(sum, 0);

	//second fifth - check if contains AGAIN - this should fail for all
	hash_batch(h, fifth, ops_arr + (fifth));
	for(int i=fifth; i < 2*fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,0);
	}

	ASSERT_EQ(hash_stop(h), 1);
	//first fifth - adding items AGAIN - all should fail - because hash was stopped
	hash_batch(h, fifth, ops_arr);
	sum=0;
	for(int i=0; i < tenth; i++){
		curr = hash_getbucketsize(h, i);
		ASSERT_EQ(curr, -1);
		sum += curr;
	}
	ASSERT_EQ(sum, -tenth);
	for(int i=0; i < fifth; i++){
		ASSERT_EQ(ops_arr[i].result ,-1);
	}


	free(vals);
	free(vals_updated);
	free(keys);
	free(ops_arr);
	//free(opts_ptrs);
	ASSERT_EQ(hash_free(h), 1);
	return true;
}

bool TestHashSync(){
	pthread_mutex_t mut;
	pthread_mutex_init(&mut, NULL);
	int res_log_idx=0;	
	hashtable h = hash_alloc(NUM_BUCKETS, hash_f);
	ASSERT_NOT_NULL(h);

	FILE * f1;
	f1 = fopen("hash_cmd.log", "w+");
	ASSERT_NOT_NULL(f1);
	fputs("#####Record Log#####\n", f1);
	fclose(f1);

	FILE * f2;
	f2 = fopen("hash_cmd_res.log", "w+");
	fputs("#####Result Log#####\n", f2);

	if (!f2) {
		printf("Error opening cmds log. errno: %d. Request canceled\n", errno);
		return false;
	}

	int fd[2];
	pipe(fd);
	int val[NUM_CMDS];
	op_t op[NUM_LOOPS][NUM_CMDS];
	pthread_t threads[NUM_CMDS*NUM_LOOPS];
	//crude mechanism to wait for last thread but it will work...
	pthread_t last;
	//unblock reads from pipe
	int flags = fcntl(fd[0], F_GETFL,0);
	fcntl(fd[0], F_SETFL, flags | O_NONBLOCK);
	for (int j=0; j<NUM_LOOPS; j++) {
		for (int i=0; i<NUM_CMDS; i++) {
			val[i] = i;
			op[j][i].compute_func = compute_f;
			op[j][i].key = rand()%MAX_KEY;
			op[j][i].val = &val[i];
			op[j][i].op = rand()%CMDS;
			last = thread_log_and_execute(h,op[j][i], fd[1], &mut);
			//read from read pipe - if something to read
			pthread_mutex_lock(&mut);
    			char buf[MAX_BUFSIZE] = {0};
			ssize_t x;
    			if ((x = read(fd[0], buf, sizeof(buf)-1)) != -1) {
				//printf("got: %d\n", x);
				buf[x] = '\0';
	       		 	fprintf(f2,buf);
	       		 }
			pthread_mutex_unlock(&mut);
			threads[j*NUM_CMDS + i] = last;
		}
		for (int i=NUM_CMDS*j; i< NUM_CMDS * (j+1); i++) {
			pthread_join(threads[i], NULL);
		}
		pthread_mutex_lock(&mut);
    		char buf[MAX_BUFSIZE] = {0};
		ssize_t x;
		while ((x = read(fd[0], buf, sizeof(buf)-1)) != -1) {
			//printf("got: %d\n", x);
			buf[x] = '\0';
		        fprintf(f2,buf);
		}
		pthread_mutex_unlock(&mut);
	}
	close(fd[0]);
	close(fd[1]);
	fclose(f2);
	pthread_mutex_destroy(&mut);
	ASSERT_EQ(hash_stop(h), 1);
	ASSERT_EQ(hash_free(h), 1);
	return true;
}


int main() {
	RUN_TEST(TestHashActions_Insert);
	RUN_TEST(TestHashActions_ContainsAndRemove);
	RUN_TEST(TestHashActions_UpdateAndCompute);
	RUN_TEST(TestHashActions_GetSizeAndStop);
	RUN_TEST(StressTestWithBatches);
	RUN_TEST(TestHashSync);
	return 0;
}
