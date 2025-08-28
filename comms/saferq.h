#ifndef SAFERQ_H
#define SAFERQ_H

#include <pthread.h>

#define MESSAGE_SIZE 255

typedef struct node {
	char message[MESSAGE_SIZE+1];
	struct node *next;
	struct node *prev;
} message_node;

/* Enqueue to the tail node; dequeue from the head node */
typedef struct queue {
	message_node *head;
	message_node *tail;
	unsigned int count;
} message_queue;

typedef struct saferqueue {
	message_queue *queue;
	pthread_mutex_t lock;
	pthread_cond_t cond;
} message_saferqueue;

message_saferqueue * message_saferqueue_initialize();
void message_enqueue(message_saferqueue *q, const char *m);
char * message_dequeue(message_saferqueue *q);

#endif /* SAFERQ_H */
