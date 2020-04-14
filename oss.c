// Pull in from shared.h
#include "shared.h"

// Functions needed
void god(int signal);
void freeMem();
void getClock();
void getSema();
void getPCB();
void getData();
void getMsg();
void createResources();
void createProcess(int index);
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

// Process Control Block Variables
key_t pcbKey = -1;
int pcbID = -1;
struct ProcessControlBlock *pcb;

// Data/Resource Variables
key_t dataKey = -1;
int dataID = -1;
struct Data *data;

// Message Queues
key_t ossMsgKey = -1;
int ossMsgID = -1;
struct Message ossMsg;

key_t usrMsgKey = -1;
int usrMsgID = -1;
struct Message usrMsg;


int main(int argc, int argv[]){
	// Set up all shared memory
	getClock();
	getSema();
	getPCB();
	getData();
	getMsg();

	// Initialize Simulated timer
	timer->sec = 0;
	timer->nsec = 0;

	// Random Number generator
	srand(time(NULL));

	// Fill allocated
	createResources();
	int i;
	/* DEBUG 
	for(i = 0; i < MAX_RESOURCE; i++){
		printf("Max resource for R%2d = %d, Shared = %d\n", i, data->resMax[i], data->isShare[i]);
	}
	*/
	for(i = 0; i < MAX_PROC; i++){
		createProcess(i);
	}
	
	/* DEBUG
	for(i = 0; i < MAX_PROC; i++){
		printf("Process P%2d needs the following resources: ", i);
		int j;
		for(j = 0; j < MAX_RESOURCE; j++){
			printf("R%2d: %2d ", j, pcb[i].maxResource[j]);
		}
		printf("\n");
	}
	*/

	// Variables for forking
	int activeChild = 0;
	pid_t pid;
	int status;
	int exitChild = 0;
	unsigned int lastSpawnSec = 0;
	unsigned int lastSpawnNSec = 0;
	bool spawnReady = true;
	

    	// Program Finished
	freeMem();

	printf("Program finished?\n");
}


void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	semctl(semaID, 0, IPC_RMID);
	shmctl(pcbID, IPC_RMID, NULL);
	shmctl(dataID, IPC_RMID, NULL);
	msgctl(ossMsgID, IPC_RMID, NULL);
	msgctl(usrMsgID, IPC_RMID, NULL);
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
		god(1);
		exit(EXIT_FAILURE);
	}
	clockID = shmget(clockKey, sizeof(struct Clock*), 0666 | IPC_CREAT);
	if(clockID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR CLOCK");
		god(1);
		exit(EXIT_FAILURE);
	}
	timer = (struct Clock*)shmat(clockID, (void*) 0, 0);
	if(timer == (void*)-1){
		perror("ERROR IN oss.c: FAILED TO ATTACH MEMEORY FOR CLOCK");
		god(1);
		exit(EXIT_FAILURE);
	}
}

void getSema(){
	semaKey = ftok("./oss.c", 2);
	if(semaKey == -1){
		perror("ERROR IN oss.c: FAILED TO GENERATE KEY FROM SHARED MEM FOR SEMAPHORE");
		god(1);
		exit(EXIT_FAILURE);
	}
	semaID = semget(semaKey, 1, 0666 | IPC_CREAT);
	if(semaID == -1){
		perror("ERROR IN oss.c: FAILED TO GET KEY FOR SEMAPHORE");
		god(1);
		exit(EXIT_FAILURE);
	}
	semctl(semaID, 0, SETVAL, 1);
}

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FROM SHARED MEEMORY FOR PCB");
		god(1);
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcessControlBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FROM SHARED MEMEORY FOR PCB");
		god(1);
		exit(EXIT_FAILURE);
	}
	pcb = shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN OSS.C: FAILED TO ATTACH MEMORY FOR PCB");
		god(1);
		exit(EXIT_FAILURE);
	}
}

void getData(){
	dataKey = ftok("./oss.c", 4);
	if(dataKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENEREATE KEY FOR DATA");
		god(1);
		exit(EXIT_FAILURE);
	}
	dataID = shmget(dataKey, sizeof(struct Data), 0666 | IPC_CREAT);
	if(dataID == -1){
		perror("ERROR IN OSS.C: FAILED TO GET KEY FOR DATA");
		god(1);
		exit(EXIT_FAILURE);
	}
	data = shmat(dataID, (void*) 0, 0);
	if(data == (void*)-1){
		perror("ERROR IN OSS.C: FAILED TO ATTACH DATA");
		god(1);
		exit(EXIT_FAILURE);
	}
}

void getMsg(){
	ossMsgKey = ftok("./oss.c", 5);
	usrMsgKey = ftok("./oss.c", 6);
	if(ossMsgKey == -1 || usrMsgKey == -1){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR MESSAGE");
		god(1);
		exit(EXIT_FAILURE);
	}
	ossMsgID = msgget(ossMsgKey, IPC_CREAT | 0600);
	usrMsgID = msgget(usrMsgKey, IPC_CREAT | 0600);
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

void createResources(){
	int i;
	for(i = 0; i < MAX_RESOURCE; i++){
		data->resMax[i] = (rand() % 10) + 1;
		data->resAvail[i] = data->resMax[i];
		int shareBin = (rand() % 9) + 1;
		if(shareBin < 2){
			data->isShare[i] = true;
		}else{
			data->isShare[i] = false;
		}
	}
}

void createProcess(int index){
	int i;
	for(i = 0; i < MAX_RESOURCE; i++){
		pcb[index].maxResource[i] = (rand() % (data->resMax[i])) + 1;
		pcb[index].allocResource[i] = 0;
		pcb[index].reqResource[i] = 0;
		pcb[index].relResource[i] = 0;

	}
}
