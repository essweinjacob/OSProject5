#include "shared.h"

// Functions used
void getMsg();
void getClock();
void getSema();
void getPCB();
void getData();
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

// PCB Variabled
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

int main(int argc, char *argv[]){
	// Get all shared memeory stuff
	getData();
	getMsg();
	getClock();
	getSema();
	getPCB();

	// Get this proceess's pid
	pid_t thisPID = getpid();
	srand(thisPID);
	int index = atoi(argv[1]);

	bool isReq = false;
	bool isAcq = false;
	bool ranSec = false;
	struct Clock userStartClock;
	struct Clock userEndClock;
	userStartClock.sec = timer->sec;
	userStartClock.nsec = timer->nsec;

	//printf("Process%2d: is in childe\n", index);
	// DEBUG printf("Have entered child\n");
	
	while(1){
		int rcvStat = msgrcv(ossMsgID, &ossMsg, (sizeof(struct Message) - sizeof(long)), getpid(), 0);
		//printf("USER: RCV ERROR: %s\n", strerror(errno));

		// Check and see if process has ran for a second
		if(!ranSec){
			userEndClock.sec = timer->sec;
			userEndClock.nsec = timer->nsec;

			if((((userEndClock.sec * 1000000000) + userEndClock.nsec) - ((userStartClock.sec * 1000000000) + userStartClock.nsec)) >= 1000000000){
				ranSec = true;
				printf("process has ran for a second\n");
			}
		}

		/* Determine what the child does
		 * 0 - Process should request resources
		 * 1 - Process should release resources
		 * 2 - Process should early term and free up resources
		 */
		bool isTerm = false;
		bool isRele = false;
		int choice;
		// If the process has run for at least a second
		if(ranSec){
			choice = rand() % 3;
		}else{
			choice = rand() % 2;
		}
		
		// Perform action
		// Request resources
		int randResource;
		if(choice == 0){
			// Make sure process isnt already requesting resources
			if(!isReq){
				// Request only a resource we need
				while(1){
					randResource = rand() % 20;
					// Makes sure we are requesting a resource we need
					if(pcb[index].maxResource[randResource] > 0){
						// Choose an amount between what we need and 1
						int requestAmount = rand() % (pcb[index].maxResource[randResource]-pcb[index].allocResource[randResource]);
						// Resources 
						pcb[index].reqResource[randResource] = requestAmount;
						isReq = true;
						usrMsg.isTerm = false;
						usrMsg.resType = randResource;
						break;
					}
				}
			}	
		}else if(choice == 1){
			// Make sure we have resources in this process before releaseing them
			if(isAcq){
				while(1){
					randResource = rand() % 20;
					// If we have more then one of a specific resource
					if(pcb[index].allocResource[randResource] > 0){
						pcb[index].relResource[randResource] = pcb[index].allocResource[randResource];
						isRele = true;
						usrMsg.isTerm = false;
						usrMsg.resType = randResource;
						break;
					}
				}
			}
		}else if(choice == 2){
			isTerm = true;
		}

		// Tell master what it did
		usrMsg.mtype = 1;
		if(isReq){
			usrMsg.isReq = true;
		}else{
			usrMsg.isReq = false;
		}
		if(isRele){
			usrMsg.isRel = true;
		}else{
			usrMsg.isRel = false;
		}
		if(isTerm){
			usrMsg.isTerm = true;
		}else{
			usrMsg.isTerm = false;
		}

		usrMsg.index = ossMsg.index;
		usrMsg.pid = ossMsg.pid;
		msgsnd(usrMsgID, &usrMsg, (sizeof(struct Message) - sizeof(long)), 0);
		//printf("USER SND ERROR: %s\n", strerror(errno));


		// What to do after telling OSS what it did
		if(isTerm){
			break;
		}else{
			// If its not going to terminate, then act on what the OSS tells it back
			// If it requested resources...
			if(isReq){
				msgrcv(ossMsgID, &ossMsg, (sizeof(struct Message) - sizeof(long)), getpid(), 0);
				//printf("USER RCV RESPONCE: %s\n", strerror(errno));
				// ... was told hit was allowed those resources
				if(ossMsg.isSafe == true){
					//printf("user request receive message received\n");
					isAcq = true;
					isReq = false;
				}
			}else if(isRele){
				isRele = false;
			}
		}
	}
	
	return index;
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

void getPCB(){
	pcbKey = ftok("./oss.c", 3);
	if(pcbKey == -1){
		perror("ERROR IN CHILD.C: FAILED TO GENERATE KEY FOR PCB");
		exit(EXIT_FAILURE);
	}
	size_t procTableSize = sizeof(struct ProcessControlBlock) * MAX_PROC;
	pcbID = shmget(pcbKey, procTableSize, 0666 | IPC_CREAT);
	if(pcbID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR PCB");
		exit(EXIT_FAILURE);
	}
	pcb = (struct ProcessControlBlock*)shmat(pcbID, (void*) 0, 0);
	if(pcb == (void*)-1){
		perror("ERROR IN USER.C: FAILED TO ATTACH MEMEORY FOR PCB");
		exit(EXIT_FAILURE);
	}
}

void getData(){
	dataKey = ftok("./oss.c", 4);
	if(dataKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR DATA");
		exit(EXIT_FAILURE);
	}
	dataID = shmget(dataKey, sizeof(struct Data), 0666 | IPC_CREAT);
	if(dataID == -1){
		perror("ERROR IN USER.C: FAILED TO GET KEY FOR DATA");
		exit(EXIT_FAILURE);
	}
	data = shmat(dataID, (void*) 0, 0);
	if(data == (void*)-1){
		perror("ERROR IN USER.C: FAILED TO ATTACH MEMEORY FOR DATA");
		exit(EXIT_FAILURE);
	}
}

void getMsg(){
	ossMsgKey = ftok("./oss.c", 5);
	usrMsgKey = ftok("./oss.c", 6);
	if(ossMsgKey == -1 || usrMsgKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUES");
		exit(EXIT_FAILURE);
	}
	ossMsgID = msgget(ossMsgKey, IPC_CREAT | 0600);
	usrMsgID = msgget(usrMsgKey, IPC_CREAT | 0600);
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
