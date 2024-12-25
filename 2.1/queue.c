#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void *qmonitor(void *arg) {
	queue_t *q = (queue_t *)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		queue_print_stats(q);
		sleep(1);
	}

	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t *q = malloc(sizeof(queue_t));
	if (q == NULL) {
		fprintf(stderr, "Cannot allocate memory for a queue\n");
		pthread_exit(NULL);
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err != EXIT_SUCCESS) {
		fprintf(stderr, "queue_init: pthread_create() failed: %s\n", strerror(err));
		pthread_exit(NULL);
	}
	
	return q;
}

void queue_destroy(queue_t *q) {
    int err;
    
    err = pthread_cancel(q->qmonitor_tid);
    if (err != EXIT_SUCCESS) {
        fprintf(stderr, "pthread_cancel error: %s\n", strerror(err));
        pthread_exit(NULL);
    }
    
    err = pthread_join(q->qmonitor_tid, NULL);
    if (err != EXIT_SUCCESS) {
        fprintf(stderr, "pthread_join error: %s\n", strerror(err));
        pthread_exit(NULL);
    }
    
    qnode_t *current = q->first;
    while (current != NULL) {
        qnode_t *tmp = current;
        current = current->next;
        free(tmp);  
    }
    
    free(q); 
    
    printf("Free memory\n");
}

int queue_add(queue_t *q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

	if (q->count == q->max_count)
		return 0;

	qnode_t *new = malloc(sizeof(qnode_t));
	if (new == NULL) {
		fprintf(stderr, "Cannot allocate memory for new node\n"); 
		pthread_exit(NULL);
	}

	new->val = val;
	new->next = NULL;

	if (q->first == NULL)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	return 1;
}

int queue_get(queue_t *q, int *val) {
	q->get_attempts++;

	assert(q->count >= 0);

	if (q->count == 0)
		return 0;

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	return 1;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
}

