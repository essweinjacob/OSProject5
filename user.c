#include "shared.h"

// Functions used
void getMsgQue();
void getClock();
void getSema();
void getPCB();
void semLock();
void semRelease();


// Global varaiables
// Clock variables
key_t clockKey = -1;
int clockID = -1;
struct Clock *timer;

// Semaphore variables
key_t semaKey = -1;
int semaID = -1;
struct sembuf semOp;

int main(int argc, int argv[]){
	// Get all shared memeory stuff
	//getMsgQue();
	getClock();
	getSema();

	return EXIT_SUCCESS;
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN child.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN child.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN child.c: FAILED TO GET KEY FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
}

void semLock(){
	semOp.sem_num = 0;
	semOp.sem_op = -1;
	semOp.sem_flg = 0;
	semop(semaID, &semOp, 1);
}

void semRelease(){
	semOp.sem_num = 0;
	semOp.sem_op = 1;
	semOp.sem_flg = 0;
	semop(semaID, &semOp, 1);
}
