#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	// Fork
#include <sys/shm.h>	// Shmget
#include <unistd.h>	// Windows stuff
#include <sys/wait.h>	// waitpid
#include <stdbool.h>	// Bool
#include <sys/msg.h>	// Shared Messaging
#include <sys/sem.h>	// Semaphores
#include <math.h>	// Rounding some numbers
#include <errno.h>	// Errno
#include <string.h>	// String
#include <stdint.h>	// uint32_t

#define MAX_PROC 18
#define MAX_FILE_LENGTH 1000000
#define TOTAL_PROC 100
#define MAX_RESOURCE 20

// Time
struct Clock{
	unsigned int sec;
	unsigned int nsec;
};

// Process Control Block
struct ProcessControlBlock{
	int index;
	pid_t pid;
	int maxResource[MAX_RESOURCE];
	int allocResource[MAX_RESOURCE];
	int reqResource[MAX_RESOURCE];
	int relResource[MAX_RESOURCE];
};

// Resource Descriptors
struct Data{
	int maxResource[MAX_RESOURCE];
	int resource[MAX_RESOURCE];
	int sharedNum;
};

// Messgae queues
struct Message{
	long mtype;
	int index;
	pid_t pid;
	int flag;
	bool isReq;
	bool isRel;
	bool isTerm;
	bool isSafe;
	int resType;
};

// Nodes for Queue
struct QNode{
	int index;
	struct QNode *next;
};

// Queue
struct Queue{
	struct QNode *front;
	struct QNode *rear;
	int count;
};

// Create Queues
struct Queue *createQueue(){
	struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = NULL;
	q->rear = NULL;
	q->count = 0;
	return q;
};

// Create Nodes for Queue
struct QNode *newNode(int index){
	struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->index = index;
	temp->next = NULL;
	return temp;
};

// Enqueue Nodes to Queue
void enQueue(struct Queue *q, int index){
	struct QNode *temp = newNode(index);
	q->count++;

	// If queue is empty then this node is both the front and the rear node
	if(q->rear == NULL){
		q->front = q->rear = temp;
		return;
	}

	// Otherwise add node to the rear
	q->rear->next = temp;
	q->rear = temp;
};

// Remove Nodes from queue
struct QNode *deQueue(struct Queue *q){
	// See if queue is empty for error checking
	if(q->front == NULL){
		return NULL;
	}

	// Move nodes around
	struct QNode *temp = q->front;
	free(temp);

	q->front = q->front->next;

	// If front becomes null, that means the queue is empty thus the rear is null as well
	if(q->front == NULL){
		q->rear = NULL;
	}

	q->count--;

	return temp;
};

// Function to see if queue is empty
bool isQueueEmpty(struct Queue *q){
	if(q->rear == NULL){
		return true;
	}else{
		return false;
	}
}

// Get Queue Count
int getQueueCount(struct Queue *q){
	return(q->count);
}

#endif
