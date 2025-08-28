#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saferq.h"

message_saferqueue * message_saferqueue_initialize() {
	/**
	 * 1. Initialize actual bytes for full message_saferqueue struct
	 * 2. Initialize individual pieces: allocation for queue, lock, and condition variable
	 */

	message_saferqueue *q = (message_saferqueue *)malloc(sizeof(message_saferqueue));
	q->queue = (message_queue *)malloc(sizeof(message_queue));
	pthread_mutex_init(&(q->lock), NULL);
	pthread_cond_init(&(q->cond), NULL);

	return q;
}

void message_enqueue(message_saferqueue *q, const char *m) {
	/** 
	 * 1. Point n's prev to tail; point n's next to NULL
	 * 2. Point tail's next to n; leave tail's prev alone
	 * 3. Point q's tail to n
	 * 4. Increment the counter
	 */

	message_node *n = (message_node*)malloc(sizeof(message_node));
	strncpy(n->message, m, MESSAGE_SIZE);
	n->message[MESSAGE_SIZE - 1] = '\0';

	/** CRITICAL REGION:
	 * 1. Acquire associated lock
	 * 2. Do work
	 * 3. Post to associated condition variable
	 * 4. Release associated lock
	 */
	pthread_mutex_lock(&(q->lock));
	n->prev = q->queue->tail;
	n->next = NULL;

	if (q->queue->count > 0) {
		q->queue->tail->next = n; /* (*q.(*tail).next) = n; */
	} else {
		q->queue->head = n;
	}
	q->queue->tail = n;
	q->queue->count += 1;

	pthread_cond_signal(&(q->cond));
	pthread_mutex_unlock(&(q->lock));
}

char * message_dequeue(message_saferqueue *q) {
	/* Extract the message */
	char *message = (char*)malloc(MESSAGE_SIZE);

	/** CRITICAL REGION:
	 * 1. Acquire the queue's lock
	 * 2. Wait on the queue's condition variable--if no work, then go to sleep.
	 * 3. When awoken, double-check there's work--if not, go back to sleep (repeat until there's work to do).
	 * 4. Do work.
	 * 5. Release the lock.
	 */
	pthread_mutex_lock(&(q->lock));
	while (q->queue->count == 0) {
		pthread_cond_wait(&(q->cond), &(q->lock));
	}

	strncpy(message, q->queue->head->message, MESSAGE_SIZE);
	/* Update q's head to be new head node */
	if (q->queue->count > 1) {
		q->queue->head = q->queue->head->next;
		/* Delete old head node */
		free(q->queue->head->prev);
		/* Break new head node's link to old head node */
		q->queue->head->prev = NULL;
	} else {
		free(q->queue->head);
		q->queue->head = NULL;
		q->queue->tail = NULL;
	}

	/* Decrement the counter */
	q->queue->count -= 1;

	pthread_mutex_unlock(&(q->lock));

	return message;
}
