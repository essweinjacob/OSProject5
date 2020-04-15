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
//key_t ossMsgKey = -1;
//int ossMsgID = -1;
//struct Message ossMsg;

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
	int i;
	bool touchedResource = false;
	bool isRequesting = false;
	bool isAcquire = false;
	struct Clock userStartClock;
	struct Clock userEndClock;
	bool ranSec = false;

	while(1){
		// Wait for message from OSS
		msgrcv(usrMsgID, &usrMsg, (sizeof(struct Message) - sizeof(long)), getpid(), 0);

		// Has the process ran for a second yet?
		if(!ranSec){
			userEndClock.sec = timer->sec;
			userEndClock.nsec = timer->nsec;
			if(((userEndClock.sec * 1000000000 + userEndClock.nsec) - (userStartClock.sec * 1000000000 + userStartClock.nsec)) >= 1000000000){
				ranSec = true;
			}
		}

		/* Have user process randomly decide what it will do
		 * 0 - Process will request resources
		 * 1 - Process will release resources
		 * 2 - Process will terminate and release all resources
		 */
		bool isTerming = false;
		bool isReleasing = false;
		int choice;
		if(!touchedResource || !ranSec){
			choice = rand() % 2;
		}else{
			choice = rand() % 3;
		}

		// If requesing
		int randomResource;
		if(choice == 0){
			touchedResource = true;
			// Make sure we arent already requesting
			if(!isRequesting){
				while(1){
					if(pcb[index].maxResource[randomResource] > 0){
						pcb[index].reqResource[randomResource] = rand() % (pcb[index].maxResource[randomResource] - pcb[index].allocResource[randomResource] + 1);
						break;
					}
				}
				isRequesting = true;
			}
		}else if(choice == 1){
			// Before we release resources, make sure it even has any
			if(isAcquire){
				for(i = 0; i < MAX_RESOURCE; i++){
					pcb[index].relResource[i] = pcb[index].allocResource[i];
				}
				isReleasing = true;
			}
		}else if(choice == 2){
			isTerming = true;
		}

		// Send message back to OSS with what the user process want to do
		// Im sure theres a better way to do this but my brain is fried
		usrMsg.mtype = 1;
		if(isTerming){
			usrMsg.isTerm = true;
		}else{
			usrMsg.isTerm = false;
		}
		if(isRequesting){
			usrMsg.isReq = true;
		}else{
			usrMsg.isReq = false;
		}
		if(isReleasing){
			usrMsg.isRel = true;
		}else{
			usrMsg.isRel = false;
		}

		msgsnd(usrMsgID, &usrMsg, (sizeof(struct Message) - sizeof(long)), 0);

		// Determine if process needs to go back to sleep or not
		if(isTerming){
			//printf("Terming in user process\n");
			break;
		}else{
			/* Update resporces depending on
			 * 1 - If request was granted
			 * 2 - If request wasnt granted
			 * 3 - If releaseing
			 */
			if(isRequesting){
				msgrcv(usrMsgID, &usrMsg, (sizeof(struct Message) - sizeof(long)), getpid(), 0);
				// Is request for resources was granted
				if(usrMsg.isSafe == true){
					for(i = 0; i < MAX_RESOURCE; i++){
						pcb[index].allocResource[i] += pcb[index].reqResource[i];
						pcb[index].reqResource[i] = 0;
					}
					isRequesting = false;
					isAcquire = true;
				}else{
					//printf("Receving that request failed\n");
					//pcb[index].reqResource[i] = 0;
					//isRequesting = false;
				}
			}
			// If we are releasing resources
			if(isReleasing){
				for(i = 0; i < MAX_RESOURCE; i++){
					pcb[index].allocResource[i] -= pcb[index].relResource[i];
					pcb[index].relResource[i] = 0;
				}
				isAcquire = false;
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
	//ossMsgKey = ftok("./oss.c", 5);
	usrMsgKey = ftok("./oss.c", 5);
	if(/*ossMsgKey == -1 ||*/ usrMsgKey == -1){
		perror("ERROR IN USER.C: FAILED TO GENERATE KEY FOR MESSAGE QUEUES");
		exit(EXIT_FAILURE);
	}
	//ossMsgID = msgget(ossMsgKey, IPC_CREAT | 0600);
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
