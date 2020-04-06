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

#define MAX_PROCESS 18

// Time
struct Clock{
	unsigned int sec;
	unsigned int nsec;
};

#endif