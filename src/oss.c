#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "shared_mem.h"
#define SEMNAME "sembach1981s"
#include <semaphore.h>
#define NANOSECOND 1000000000
#define LOGFILESIZE 100000

key_t shmKey, resKey, pcbKey, req_resKey;
int shmId, resId, pcbshmId,req_resId, log_lines = 0;
shared_mem *clock;
pcb *pcbs;
resource *resources;
req_res *request;
FILE *logfile;
char *file_name;
sem_t *mySemaphore;
int i,m,k,x,k,j,status,p,q;


void clearSharedMemory() {
//fprintf(stderr, "Total Children Forked : %d \n", child_count);
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)resources);
shmdt((void *)pcbs);
shmdt((void *)request);
//fprintf(stderr,"Closing File \n");
//fclose(logfile);
fprintf(stderr, "OSS started detaching all the shared memory segments \n");
shmctl(shmId, IPC_RMID, NULL);
shmctl(resId, IPC_RMID, NULL);
shmctl(pcbshmId, IPC_RMID, NULL);
shmctl(req_resId, IPC_RMID, NULL);
sem_unlink(SEMNAME);
fprintf(stderr, "Unlinked Semaphore \n");
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}

/* void killExistingChildren(){
for(k=0; k<18; k++)
{
if(process_control_blocks[k].processId != 0)
{
kill(process_control_blocks[k].processId, SIGTERM);
}
}
} */

int randomNumberGenerator(int min, int max)
{
	return ((rand() % (max-min +1)) + min);
}

void initialize_resources(){
srand(time(NULL));
int i;
	for(i = 0; i < 20; i++)
	{
		srand(i++);
		resources[i].total_instances = randomNumberGenerator(1,10);
		resources[i].avail_instances = resources[i].total_instances;
	}
}

void myhandler(int s) {
if(s == SIGALRM)
{
fprintf(stderr, "Master Time Done\n");
//killExistingChildren();
clearSharedMemory();
}

if(s == SIGINT)
{
fprintf(stderr, "Caught Ctrl + C Signal \n");
fprintf(stderr, "Killing the Existing Children in the system \n");
//killExistingChildren();
clearSharedMemory();
}
exit(1);
}

int q;
int getPCBBlockId(int processId)
{
        for(q = 0; q<18;q++)
        {
                if(pcbs[q].processId == processId)
                return q;
        }
}

int main (int argc, char *argv[]) {

while((x = getopt(argc,argv, "hs:l:")) != -1)
switch(x)
{
case 'h':
        fprintf(stderr, "Usage: %s -l logfile_name  -h [help]\n", argv[0]);
        return 1;
case 'l':
	file_name = optarg;
	break;
case '?':
        fprintf(stderr, "Please give '-h' for help to see valid arguments \n");
        return 1;
}

signal(SIGALRM, myhandler);
alarm(2);
signal(SIGINT, myhandler);

shmKey = ftok(".", 'c');
shmId = shmget(shmKey, sizeof(shared_mem), IPC_CREAT | 0666);

if(shmId <0 )
{
	fprintf(stderr, "Error in shmget \n");
	exit(1);
}

clock = (shared_mem*) shmat(shmId, NULL, 0);

fprintf(stderr, "Allocated Shared Memory For OSS Clock \n");

int pcbshmSize = sizeof(pcbs) * 18;
pcbKey = ftok(".", 'x');
pcbshmId = shmget(pcbKey, pcbshmSize, IPC_CREAT | 0666);
if(pcbshmId < 0)
{
	fprintf(stderr, "Error in Shmget for PCB's \n");
	exit(1);
}

pcbs = (pcb*) shmat(pcbshmId, NULL, 0);
if(pcbs == (void *) -1)
{
	perror("Error in attaching Memory to process_control_blocks \n");
	exit(1);
}


int resources_size = sizeof(resources) * 20;

resKey = ftok(".bac", 's');
resId = shmget(resKey, sizeof(resources_size), IPC_CREAT | 0666);
if(resId < 0)
{
	fprintf(stderr, "Error in Shmget for resources \n");
	exit(1);
}

resources = (resource*) shmat(resId, NULL, 0);
if(resources == (void *) -1)
{
	perror("Error in attaching Memory to resources \n");
	exit(1);
}

req_resKey = ftok(".", 'b');
req_resId = shmget(req_resKey, sizeof(req_res), IPC_CREAT | 0666);

if(req_resId <0 )
{
        fprintf(stderr, "Error in shmget from Request Struct \n");
        exit(1);
}

request = (req_res*) shmat(req_resId, NULL, 0);


mySemaphore = sem_open(SEMNAME, O_CREAT, 0666,0);

fprintf(stderr, "Created Semaphore with Name %s \n", SEMNAME);

clock -> seconds = 0;
clock -> nanoseconds = 0;

/*if(file_name == NULL)
file_name = "default";
logfile = fopen(file_name, "w"); */

srand(time(NULL));
for(i = 0; i < 18; i++)
{
	pcbs[i].processId = 0;
	pcbs[i].request = 0;
	pcbs[i].release = 0;
	pcbs[i].terminate = 0;
	int j;
	for(j = 0; j < 20; j++) {
	pcbs[i].maxclaim.qty[j] = 0;
	pcbs[i].allocation.qty[j] = 0;
	}
}

int currentPCBBlock = -1,k, randomvalue, next_child_time = 500000000;
pid_t mypid;
initialize_resources();

while(1)
{
	currentPCBBlock = -1;
	for(k = 0; k<18; k++)
	{
		if(pcbs[k].processId == 0)
		{
		currentPCBBlock = k;
		break;
		}
	}
	if(currentPCBBlock >= 0 && (clock -> nanoseconds >= next_child_time))
	{
		if((mypid = fork()) ==0)
		{
			char argument1[50], argument2[20],argument3[50], argument4[20], argument5[20];
                	char *s_val = "-s";
			char *pcbshmVal2 = "-j";
			char *semVal = "-k";
			char *res_val = "-p";
			char *req_val = "-b";
			char *arguments[] = {NULL,res_val,argument2,s_val, argument3,pcbshmVal2, argument4,semVal, argument5,req_val, argument1, NULL};	
			arguments[0]="./user";
			sprintf(arguments[2], "%d", resId);
			sprintf(arguments[4], "%d", shmId);
               		sprintf(arguments[6], "%d", pcbshmId);
			sprintf(arguments[8], "%s", SEMNAME);
			sprintf(arguments[10], "%d", req_resId);	
			execv("./user", arguments);
                	fprintf(stderr, "Error in exec");
			exit(0); 
		}
	}
	
	pcbs[currentPCBBlock].processId = mypid;
	int p;
	for(p = 0; p<20; p++)
	{
		int claim = rand() % 3 + 1;
		pcbs[currentPCBBlock].maxclaim.qty[p] = claim;
	}	
	
	clock -> nanoseconds += 1;
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
	}
	
	/*if(request -> processNumber >= 0)
	{
		int blockId = getPCBBlockId(processNumber);
		if(pcbs[blockId].request >= 0)
		{
			fprintf(stderr, "Found a request from %d process for %d resource \n",request -> processNumber, pcbs[blockId].request);
		int resInd = pcbs[blockId].request;
		int request = pcbs[blockId].maxclaim.qty[resInd] - pcbs[blockId].allocation.qty[resInd];
		if(request > resources[resInd].avail_instances)
		enqueue(queue, request -> processNUmber);
		else{
		
		}	
		} // Request resource
		
		if(pcbs[blockId].release >= 0)
		{
			int resInd = pcbs[blockId].release;
			resources[resInd].avail_instances += pcbs[blockId].allocation.qty[resInd];
		}
	} */
}
wait(NULL);

clearSharedMemory();
return 0;

}
