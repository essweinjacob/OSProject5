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
void createProcess(int index, int pid);
void semLock();
void semRelease();
void incTimer();
void spawnCheck();
void setMat(int max[][MAX_RESOURCE], int alloc[][MAX_RESOURCE], int count);
void calcNeedMat(int need[][MAX_RESOURCE], int max[][MAX_RESOURCE], int alloc[][MAX_RESOURCE], int count);
void displayArr(char *action, char *lName, int arr[MAX_RESOURCE]);
void displayMat(char *matrixName, int matrix[][MAX_RESOURCE], int count);
bool bankers(int index);

// Global Variables
// Pid list info
int *listOfPIDS;
int forkIndex = -1;

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

// Spawn Ready variables
bool spawnReady = true;
unsigned int lastSpawnSec = 0;
unsigned int lastSpawnNSec = 0;

// Queue
static struct Queue *queue;

// Verbose and line count are global because im lazy
bool verbose = false;
int lineCount = 0;

// File Variables
//FILE *fLOG = NULL;


int main(int argc, char *argv[]){
	// Set up all shared memory
	getClock();
	getSema();
	getPCB();
	getData();
	getMsg();

	// Set up signal handling
	signal(SIGINT, god);
	signal(SIGPROF, god);

	// Getopt stuff
	int opt;
	while((opt = getopt(argc, argv, "hv")) != -1){
		switch(opt){
			case 'h':
				printf("---HELP MENU---\n");
				printf("-h: Prints help menu\n");
				printf("-v: Turns on verbose (off by default)\n");
				exit(0);
			case 'v':
				verbose = true;
				break;
			default:
				printf("Please use -h for help menu you twit\n");
				exit(EXIT_FAILURE);
		}
	}

	// Make sure there arent any extra arguments
	if(optind < argc){
		printf("OSS ERROR EXTRA ARGUMENTS GIVEN LOOK AT -h FOR HELP MENU PLEASE\n");
		exit(EXIT_FAILURE);
	}
	/*
	// Open File log
	fLOG = fopen("resourceActivityLog", "w");
	if(fLOG = NULL){
		perror("OSS FAILED TO OPEN RESOURCE ACTVITY LOG");
		exit(EXIT_FAILURE);
	}
	*/

	// Random Number generator
	srand(time(NULL));

	// Fill allocated
	createResources();
	int i;

	// Variables for forking
	int activeChild = 0;
	pid_t pid;
	int exitCount = 0;
	int maxLaunch = 0;
	bool newProc = false;
	listOfPIDS = calloc(TOTAL_PROC, sizeof(int));
	
	// This will be used to see if a proc index is in use
	bool procAvail[18];
	for(i = 0; i < MAX_PROC; i++){
		procAvail[i] = true;
	}


	// Initialize simulated timer
	timer->sec = 0;
	timer->nsec = 0;
	
	// Setup Queue to put processes inside of
	queue = createQueue();
	
	while(1){
		// Generate Fork index by getting the remainder of current fork index + 1 / 18
		forkIndex = (forkIndex + 1) % MAX_PROC;
		//printf("Fork Index: %d\n", forkIndex);
		//Check if ready to fork
		if(spawnReady == true && activeChild < MAX_PROC && exitCount < TOTAL_PROC && maxLaunch < TOTAL_PROC && procAvail[forkIndex] == true){
			pid = fork();
			// On failure to fork
			if(pid < 0){
				perror("FAILED TO FORK");
				god(1);
				exit(EXIT_FAILURE);
			}
			// Create Child
			if(pid == 0){
				char convIndex[1024];
				sprintf(convIndex, "%d", forkIndex);
				char *args[] = {"./user", convIndex, NULL};
				int exeStatus = execvp(args[0], args);
				if(exeStatus == -1){
					perror("FAILED TO LAUNCH CHILD\n");
					god(1);
				}
			}else{
				// In parent
				printf("P%d has launched at %d:%d\n", forkIndex, timer->sec, timer->nsec);
				//fprintf(fLOG, "P%d has launched at %d:%d\n", forkIndex, timer->sec, timer->nsec);
				procAvail[forkIndex] = false;
				listOfPIDS[forkIndex] = pid;
				activeChild++;
				maxLaunch++;
				spawnReady = false;
				
				// Record Spawn time
				lastSpawnSec = timer->sec;
				lastSpawnNSec = timer->nsec;

				// Put details into pcb and message
				createProcess(forkIndex, pid);
				enQueue(queue, forkIndex);
			}
		}/*DEBUG
		else{
			printf("Whyd we skip a fork? spawnReady = %d, activeChild = %d, exitCount = %d, maxLaunch = %d, procAvail[%d] = %d\n", spawnReady, activeChild, exitCount, maxLaunch, forkIndex, procAvail[forkIndex]);
		}*/

		// Go through Nodes/Processes to see what they want if anything
		struct QNode next;
		struct Queue *trackQueue = createQueue();	// Create a temporary queue

		int currentIter = 0;
		//printf("here?\n");
		next.next = queue->front;
		// While the next item isn't empty / while a process still exists
		while(next.next != NULL){
			incTimer();

			int childIndex = next.next->index;
			// Give details to message
			ossMsg.mtype = ossMsg.pid = pcb[childIndex].pid;
			ossMsg.index = childIndex;
			// Send message to child saying its their turn
			msgsnd(ossMsgID, &ossMsg, (sizeof(struct Message) - sizeof(long)), 0);

			// Wait for a responce from the child
			msgrcv(ossMsgID, &ossMsg, (sizeof(struct Message) - sizeof(long)), 1, 0);

			incTimer();
			
			// If the process wants to terminate
			if(ossMsg.isTerm == true){
				printf("OSS P%d has finished running at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
				//fprintf(fLOG, "OSS P%d has finished running at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
				// Remove itself from the queue and find next node
				struct QNode current;
				current.next = queue->front;
				while(current.next != NULL){
					if(current.next->index != childIndex){
						enQueue(trackQueue, current.next->index);
					}

					// Point to next available process
					if(current.next->next != NULL){
						current.next = current.next->next;
					}else{
						current.next = NULL;
					}
				}
				// Reassign all the queues
				while(!isQueueEmpty(queue)){
					deQueue(queue);
				}
				while(!isQueueEmpty(trackQueue)){
					int i = trackQueue->front->index;
					enQueue(queue, i);
					deQueue(trackQueue);
				}

				// Point to next process/queue element
				next.next = queue->front;
				int i;
				for(i = 0; i < currentIter; i++){
					if(next.next->next != NULL){
						next.next = next.next->next;
					}else{
						next.next = NULL;
					}
				}
				continue;
			}

			incTimer();
			
			// If the user process is requesting resources
			if(ossMsg.isReq == true){
				printf("OSS P%d is requesting resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
				//fprintf(fLOG, "OSS P%d is requesting resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);

				bool isSafe = bankers(childIndex);
				
				incTimer();

				// Send message to user process saying its safe to request resources or not
				ossMsg.mtype = pcb[childIndex].pid;
				if(isSafe){
					ossMsg.isSafe = true;
					printf("OSS P%d has been granted resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
					//fprintf(fLOG, "OSS P%d has been granted resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
				}else{
					ossMsg.isSafe = false;
					printf("OSS P%d has been denied resources at time %d:%d. Putting it to sleep.\n", ossMsg.index, timer->sec, timer->nsec);
					//fprintf(fLOG, "OSS P%d has been denied resources at time %d:%d. Putting it to sleep.\n", ossMsg.index, timer->sec, timer->nsec);
				}
				msgsnd(ossMsgID, &ossMsg, (sizeof(struct Message) - sizeof(long)), 0);
			}

			incTimer();

			// If user process is releaseing resources
			if(ossMsg.isRel){
				printf("OSS P%d is releaseing resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
				//fprintf(fLOG, "OSS P%d is releaseing resources at time %d:%d\n", ossMsg.index, timer->sec, timer->nsec);
			}
			
			currentIter++;
			// Point to next element
			if(next.next->next != NULL){
				next.next = next.next->next;
			}else{
				next.next = NULL;
			}
		}
		//printf("Did we escape???\n");
		// Free the temporary queue
		free(trackQueue);
		incTimer();

		// Check if child has ended
		int status;
		pid_t childPid = waitpid(-1, &status, WNOHANG);
		if(childPid > 0){
			int returnIndex = WEXITSTATUS(status);
			printf("P%2d has exited at time %2d:%9d\n", returnIndex, timer->sec, timer->nsec);
			//fprintf(fLOG, "P%2d has exited at time %2d:%9d\n", returnIndex, timer->sec, timer->nsec);
			procAvail[returnIndex] = true;
			activeChild--;
			exitCount++;
			if(exitCount >= TOTAL_PROC && activeChild == 0){
				break;
			}
		}

		// DEBUG printf("Active children  = %d\n", activeChild);
		// Fail safe for forking to prevent fork bomb
		if(activeChild >= MAX_PROC){
			perror("FORK BOMB PREVENTED");
			god(1);
		}
		
		incTimer();
		// Check to see if enough time has passed for another process to spawn
		if(spawnReady == false){
			spawnCheck();
		}
		
		/* DEBUG
		printf("Exit count: %d\n", exitCount);
		printf("Active Childred: %d\n", activeChild);
		*/
	}

    	// Program Finished
	freeMem();
}


void freeMem(){
	shmctl(clockID, IPC_RMID, NULL);
	semctl(semaID, 0, IPC_RMID);
	shmctl(pcbID, IPC_RMID, NULL);
	shmctl(dataID, IPC_RMID, NULL);
	msgctl(ossMsgID, IPC_RMID, NULL);
	//msgctl(usrMsgID, IPC_RMID, NULL);
	free(listOfPIDS);
	//fclose(fLOG);
}

void god(int signal){
	int i;
	for(i = 0; i < MAX_PROC; i++){
		kill(listOfPIDS[i], SIGTERM);
	}
	printf("GOD HAS BEEN CALLED AND THE RAPTURE HAS BEGUN. SOON THERE WILL BE NOTHING\n");
	//fprintf(fLOG, "GOD HAS BEEN CALLED AND THE RAPTURE HAS BEGUN. SOON THERE WILL BE NOTHING\n");
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
	//usrMsgKey = ftok("./oss.c", 6);
	if(ossMsgKey == -1 /*|| usrMsgKey == -1*/){
		perror("ERROR IN OSS.C: FAILED TO GENERATE KEY FOR MESSAGE");
		god(1);
		exit(EXIT_FAILURE);
	}
	ossMsgID = msgget(ossMsgKey, IPC_CREAT | 0600);
	//usrMsgID = msgget(usrMsgKey, IPC_CREAT | 0600);
}

/*
 * This process puts a semaphore lock on the timer, increases the time by a random amount between 1000 - 1 and checks in enough
 * microsconds have passed to equal a second and does math for it
 */
void incTimer(){
	semLock();

	int nsecInc = (rand() % 1000) + 1;
	timer->nsec += nsecInc;
	while(timer->nsec >= 1000000000){
		timer->sec++;
		timer->nsec -= 1000000000;
		// DEBUG printf("Stuck in timer?\n");
	}

	semRelease();
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

/*
 * This function creates all resource descriptors
 */
void createResources(){
	int i;
	for(i = 0; i < MAX_RESOURCE; i++){
		// Amount of a resource is determined randomly [1-10]
		data->maxResource[i] = (rand() % 10) + 1;
		data->resource[i] = data->maxResource[i];
		int shareBin = (rand() % 9) + 1;
		// If random number is between [0-2] the resource is shared, [3-8] it is not
		if(shareBin < 2){
			data->sharedNum++;
		}
	}
}

/*
 * This function creates processes
 */
void createProcess(int index, pid_t pid){
	int i;
	pcb[index].index = index;
	pcb[index].pid = pid;
	for(i = 0; i < MAX_RESOURCE; i++){
		pcb[index].maxResource[i] = (rand() % (data->maxResource[i])) + 1;
		pcb[index].allocResource[i] = 0;
		pcb[index].reqResource[i] = 0;
		pcb[index].relResource[i] = 0;
	}
}

/*
 * This function checks if enough time has passed for a new process to start
 * 1000 nanoseconds = 1 microsecond
 */
void spawnCheck(){
	int randomTime = (rand() % (500000 - 1000 + 1)) + 1000;
	if((((timer->sec * 1000000000) + timer->nsec) - ((lastSpawnSec * 1000000000) + lastSpawnNSec)) >= randomTime){
		spawnReady = true;
	}
}

/*
 * Creates matri(es?)
 */
void setMat(int max[][MAX_RESOURCE], int alloc[][MAX_RESOURCE], int count){
	struct QNode next;
	next.next = queue->front;

	int i, j;

	int childIndex = next.next->index;
	// Copy values into max and alloc matri so we have a more convient names
	for(i = 0; i < count; i++){
		for(j = 0; j < MAX_RESOURCE; j++){
			max[i][j] = pcb[childIndex].maxResource[j];
			alloc[i][j] = pcb[childIndex].allocResource[j];
		}

		// Point to next element
		if(next.next->next != NULL){
			next.next = next.next->next;
			childIndex = next.next->index;
		}else{
			next.next = NULL;
		}
	}
}

/*
 * This function calculates how much of each resource a process needs
 */
void calcNeedMat(int need[][MAX_RESOURCE], int max[][MAX_RESOURCE], int alloc[][MAX_RESOURCE], int count){
	int i, j;
	for(i = 0; i < count; i++){
		for(j = 0; j < MAX_RESOURCE; j++){
			// Math to find need
			need[i][j] = max[i][j] - alloc[i][j];
		}
	}
}

/*
 * This diplays the Available and Request Arrays
 */
void displayArr(char *action, char *lName, int arr[MAX_RESOURCE]){
	printf("---%s Resource---\n%3s : ", action, lName);
	//fprintf(fLOG, "---%s Resource---\n%3s : ", action, lName);
	int i;
	// Print for each resource
	for(i = 0; i < MAX_RESOURCE; i++){
		printf("%2d", arr[i]);
		//fprintf(fLOG, "%2d", arr[i]);
		if(i < MAX_RESOURCE - 1){
			printf(" | ");
			//fprintf(fLOG, " | ");
		}
	}
	printf("|\n");
	//fprintf(fLOG, "|\n");
}

/*
 * This function displays the Maximum and Allocated Resources Matri
 */

void displayMat(char *matrixName, int matrix[][MAX_RESOURCE], int count){
	struct QNode next;
	next.next = queue->front;

	int i, j;
	printf("---%s Matrix---\n", matrixName);
	//fprintf(fLOG, "---%s Matrix---\n", matrixName);

	for(i = 0; i < count; i++){
		printf("P%2d : ", next.next->index);
		//fprintf(fLOG, "P%2d : ", next.next->index);
		for(j = 0; j < MAX_RESOURCE; j++){
			printf("%2d", matrix[i][j]);
			//fprintf(fLOG, "%2d", matrix[i][j]);
			if(j < MAX_RESOURCE - 1){
				printf(" | ");
				//fprintf(fLOG, " | ");
			}
		}
		printf("|\n");
		//fprintf(fLOG, "|\n");

		// Point to next element
		if(next.next->next != NULL){
			next.next = next.next->next;
		}else{
			next.next = NULL;
		}
	}
}

bool bankers(int index){
	/*
	 * i - Used for lloping
	 * k - Used to find an unfinished process with resources available
	 * j - Checks if resources of current process is < work
	 * l - Add allocated resources of current process to available resources
	 */
	int i, j, k, l;

	// Make sure the queue is not empty
	struct QNode next;
	next.next = queue->front;
	if(next.next == NULL){
		return true;
	}

	int count = getQueueCount(queue);	// Amount of procs in queue
	int max[count][MAX_RESOURCE];		// Maxium Matrix
	int alloc[count][MAX_RESOURCE];		// Allocated Resource in a Proc
	int request[MAX_RESOURCE];		// Resources Requested by a Proc
	int need[count][MAX_RESOURCE];		// Resources needed by a Proc
	int available[MAX_RESOURCE];		// Resources Available by a particular resource descriptor

	// Set up matrix given resources, queue, and pcb
	setMat(max, alloc, count);
	// Calulate matrix needs
	calcNeedMat(need, max, alloc, count);

	// Set up avail and request arrays
	for(i = 0; i < MAX_RESOURCE; i++){
		available[i] = data->resource[i];
		request[i] = pcb[index].reqResource[i];
	}
	// Update available array
	for(i = 0; i < count; i++){
		for(j = 0; j < MAX_RESOURCE; j++){
			available[j] -= alloc[i][j];
		}
	}

	// Map pcb index to need array
	int id = 0;
	next.next = queue->front;
	while(next.next != NULL){
		if(next.next->index == index){
			break;
		}
		id++;

		// Point to next element
		if(next.next->next != NULL){
			next.next = next.next->next;
		}else{
			next.next = NULL;
		}
	}

	// Display information
	if(verbose){
		displayMat("Maximum", max, count);
		displayMat("Allocation", alloc, count);
		char str[1024];
		sprintf(str, "P%2d", index);
		displayArr("Request", str, request);
	}

	// Try to find a "Safe Sequence"
	bool finish[count];	// To store finsih
	int safeSeq[count];	// Store the path of safe sequence
	memset(finish, 0, count * sizeof(finish[0]));	// Mark all processes as not finished


	// Make copy of available resources
	int work[MAX_RESOURCE];
	for(i = 0; i < MAX_RESOURCE; i++){
		work[i] = available[i];
	}

	// Request Algorithm
	for(j = 0; j < MAX_RESOURCE; j++){
		if(need[id][j] < request[j] /*&& j < data->sharedNum*/){
			//printf("ASKED FOR MORE THEN MAX REQUEST\n");
			if(verbose){
				displayArr("Available","A ", available);
				displayMat("Need", need, count);
			}
			return false;
		}

		if(request[j] <= available[j] /*&& j < data->sharedNum*/){
			available[j] -= request[j];
			alloc[id][j] += request[j];
			need[id][j] -= request[j];
		}else{
			//printf("NOT ENOUGH RESOURCES AVAILABLE\n");
		
			if(verbose){
				displayArr("Available", "A ", available);
				displayMat("Need", need, count);
			}
			return false;
		}
	}

	// Safety Algoirthm
	int inde = 0;
	while(inde < count){
		bool found = false;
		for(k = 0; k < count; k++){
			if(finish[k] == 0){
				for(j = 0; j < MAX_RESOURCE; j++){
					if(need[k][j] > work[j] && data->sharedNum){
						break;
					}
				}
				// If k was satisfied
				if(j == MAX_RESOURCE){
					for(l = 0; l < MAX_RESOURCE; l++){
						work[l] += alloc[k][l];
					}
					// Add proc to safe sequence
					safeSeq[inde++] = k;

					// Mark k as finished
					finish[k] = 1;
					found = true;
				}
			}
		}
		// If we couldn't find a proces sfor safe sequence
		if(found == false){
			//printf("SYSTEM IS UNSAFE\n");
			return false;
		}
	}

	// Display information again
	if(verbose){
		displayArr("Available", "A ", available);
		displayMat("Need", need, count);
	}

	// Map safe sequence to queue sequence
	int seq[count];
	int seqIndex = 0;
	next.next = queue->front;
	while(next.next != NULL){
		seq[seqIndex++] = next.next->index;
		// Point to next element
		if(next.next->next != NULL){
			next.next = next.next->next;
		}else{
			next.next = NULL;
		}
	}
	/*
	// If in safe sequence then print the sequence
	printf("OSS HAS FOUND A SAFE SEQUENCE IT IS: ");
	for(i = 0; i < count; i++){
		printf("%2d ", seq[safeSeq[i]]);

	}
	printf("\n");
	return true;
	*/
}
