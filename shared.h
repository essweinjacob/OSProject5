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

#define MAX_PROC 18
#define MAX_FILE_LENGTH 1000000
#define TOTAL_PROC 100
#define MAX_RESOURCE 20
#define MIN_SHARED_RESOURCES 15
#define MAX_SHARED_RESOURCES 25

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
	int allocatedResource[MAX_RESOURCE];
	int requestResource[MAX_RESOURCE];
	int releasedResource[MAX_RESOURCE];
};

struct Data{
	int resource[MAX_RESOURCE];
	int sharedResource[MAX_RESOURCE];
};

#endif
