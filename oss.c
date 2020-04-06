// Pull in from shared.h
#include "shared.h"

// Functions needed
void god(int signal);
void freeMem();
void getClock();
void getSema();
void semLock();
void semRelease();
void incTimer();

// Global Variables
// Pid list info
int *listOfPIDS;
int numOfPIDS = 0;

// Clock variables
key_t clockKey = -1;
int clockID = -1;
struct Clock *timer;

// Semaphore variables
key_t semaKey = -1;
int semaID = -1;
struct sembuf semOp;


int main(int argc, int argv[]){
	// Set up all shared memory
	getClock();
	getSema();

    // Program Finished
	freeMem();

	printf("Program finished?\n");
}


void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	shmctl(semaID, IPC_RMID, NULL);
	free(listOfPIDS);
}

void god(int signal){
	int i;
	for(i = 0; i < numOfPIDS; i++){
		kill(listOfPIDS[i], SIGTERM);
	}
	printf("GOD HAS BEEN CALLED AND THE RAPTURE HAS BEGUN. SOON THERE WILL BE NOTHING\n");
	freeMem();
	kill(getpid(), SIGTERM);
}

void getClock(){
	clockKey = ftok("./oss.c", 1);
	if(clockKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN oss.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR SEMAPHORE");
		exit(EXIT_FAILURE);
	}
	semctl(semaID, 0, SETVAL, 1);
}

void incTimer(){
	timer->nsec += 10000;
	while(timer->nsec >= 1000000000){
		timer->sec++;
		timer->nsec -= 1000000000;
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
