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

struct Data{
	int resMax[MAX_RESOURCE];
	int resAvail[MAX_RESOURCE];
	bool isShare[MAX_RESOURCE];
};

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

#endif
