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
#include <sys/msg.h>
#include "priority_queue.h"
#define NANOSECOND 1000000000
#define LOGFILESIZE 100000

int status,x,i,m,n,p,q;
key_t clockKey,shmKey, stateKey, msgqueueKey;
int clockId, stateId, shmId, msgqueueId;
logical_clock *clock;
res_desc *system_state;
FILE *logfile;
char *file_name;
char *res_claims[21];
int child_pids[18];
long seed_value ;
int bit_vector[18];

void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)system_state);
//fprintf(stderr,"Closing File \n");
//fclose(logfile);
fprintf(stderr, "OSS started detaching all the shared memory segments \n");
shmctl(clockId, IPC_RMID, NULL);
shmctl(stateId, IPC_RMID, NULL);
msgctl(msgqueueId, IPC_RMID, NULL);
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}

void printInfo()
{
int p, q;
int i;
	fprintf(stderr, "Resources : ");
	for (i = 0; i < 20; i++)
        {
		fprintf(stderr, "%d  ", system_state -> resources[i]);	
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Available : ");
        for (i = 0; i < 20; i++)
        {
		fprintf(stderr, "%d  ", system_state -> available[i]);
        }
	printf("\n");

for (p =0; p< 18;p++)
{
        for(q=0; q< 20; q++)
        {
        fprintf(stderr, "%d ", system_state -> allocated[p][q]);
	//fprintf(stderr, "%d ", system_state -> available[q]);
        if(q == 19)
        fprintf(stderr, "\n");
        }
}
}

void killExistingChildren(){
int k;
for(k=0; k<18; k++)
{
if(child_pids[k] > 0)
{
kill(child_pids[k], SIGTERM);
}
}
}

void myhandler(int s) {
printInfo();
if(s == SIGALRM)
{
fprintf(stderr, "Master Time Done\n");
killExistingChildren();
clearSharedMemory();
}

if(s == SIGINT)
{
fprintf(stderr, "Caught Ctrl + C Signal \n");
fprintf(stderr, "Killing the Existing Children in the system \n");
killExistingChildren();
clearSharedMemory();
}
exit(1);
}

int checkIfSafeState(res_desc system_state)
{
	int currentavailable[20];
	int m,k = 0,q = 0, resource_count = 0, process_index;
	for(m = 0; m < 18; m++)
	{
		if(bit_vector[m] == 1)
		 q++;
	}
	int running[q];
		
	for(m = 0; m < 18; m++)
	{
		if(bit_vector[m] == 1)
		{
		running[k] = m;
		k++;
		}
	}

	int i,j;
	for(i = 0; i < 20; i++)
		currentavailable[i] = system_state.available[i];
	
	int possible = 1, found = 0, pid_location; 
	while(possible)
	{
		for(j = 0; j < q; j++)
		{
		int temp = running[j];
		if(temp != -1)
		{
			resource_count = 0;
			for(i = 0; i < 20; i++)
			{
				if(system_state. maxclaims[temp][i] - system_state.allocated[temp][i] <= currentavailable[i])
				resource_count++;
			}
			if(resource_count == 20)
			{
			process_index = temp;
			found = 1;
			break;
			}
		}
		}
		if(found)
		{
			int n;
			for(n = 0; n < 20; n++)
			{
			currentavailable[n] = currentavailable[n] + system_state.allocated[process_index][n];
			running[j] = -1;
			found = 0;
			}
		}
		else
		possible = 0;	
	}
	 int x, notdone = 0;
	for(x = 0; x< q; x++)
	{
		if(running[x] >= 0)
		notdone = 1;
	}
	if(notdone == 1)
		return 0;
	else 
		return 1;
	
} 

void initialize_resources(){
//srand(time(NULL));
int i;
	for(i = 0; i < 20; i++)
	{
		srand(seed_value++);
		system_state -> resources[i] = rand() % 10 + 1;
		system_state -> available[i] = system_state -> resources[i];
	}
}

int getIndexById(int processId)
{
	int l;
	for(l = 0; l < 18; l++)
	{
		if(child_pids[l] == processId)
		return l;
	}
}

void generateresourceclaims(int pindex)
{
	int p = 0,q, maxres;
	for(q = 0; q < 20; q++)
	{
		char array[sizeof(int)];
		srand(seed_value++);
		maxres = system_state -> resources[q];
		system_state -> maxclaims[pindex][q] = rand() % maxres;
		snprintf(array, sizeof(int), "%d", system_state -> maxclaims[pindex][q]);
		res_claims[q]= malloc(sizeof(array));
		strcpy(res_claims[q], array);
	}
}

int main (int argc, char *argv[]) {
seed_value = time(NULL);
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

Queue* process = create_queue(20);
Queue* resource = create_queue(20);

signal(SIGALRM, myhandler);
alarm(2);
signal(SIGINT, myhandler);

clockKey = ftok(".", 'c');
clockId = shmget(clockId, sizeof(logical_clock), IPC_CREAT | 0666);
if(clockId <0 )
{
	fprintf(stderr, "Error in shmget for Clock \n");
	exit(1);
}

clock = (logical_clock*) shmat(clockId, NULL, 0);
fprintf(stderr, "Allocated Shared Memory For OSS Clock \n");


stateKey = ftok(".bac", 's');
stateId = shmget(stateKey, sizeof(res_desc), IPC_CREAT | 0666);
if(stateId < 0)
{
	fprintf(stderr, "Error in Shmget for State \n");
	exit(1);
}

system_state = (res_desc*) shmat(stateId, NULL, 0);
if(system_state == (void *) -1)
{
	perror("Error in attaching Memory to State\n");
	exit(1);
}

fprintf(stderr, "Allocated Shared Memory For State Struct \n");

msgqueueKey = ftok(".", 'p');
msgqueueId = msgget(msgqueueKey, IPC_CREAT | 0666);

pid_t mypid;
int n;
for(n = 0;n < 18; n++)
{
	child_pids[n] = 0;
	bit_vector[n] = -1;
}

initialize_resources();

int m; 
for(m = 0; m< 20; m++)
	fprintf(stderr, "%d ", system_state -> resources[m]);
fprintf(stderr, "\n");
fprintf(stderr, "---------------------------------------------------------- \n");


int index = -1,b, next_child_time_nano, next_child_time_sec;
while(1)
{
	index = -1;
	for(b = 0; b< 18; b++)
	{
		if(child_pids[b] == 0)
		{
		index = b;
		break;
		}
	}
		if((index >= 0) && (next_child_time_sec >= clock -> seconds && next_child_time_nano >= clock -> nanoseconds))
		{
			generateresourceclaims(index);
			if((mypid = fork()) ==0)
			{
			char argument2[20],argument3[50], argument4[20], argument5[20];
			sprintf(argument2, "%d", clockId);
			sprintf(argument3, "%d", msgqueueId);
			sprintf(argument5, "%d", stateId);
			execle("./user", argument2, argument3, argument5, NULL ,res_claims );
                	fprintf(stderr, "Error in exec");
			exit(0); 
			}
		child_pids[index] = mypid;
		bit_vector[index] = 1;
		}

	 srand(seed_value++);
        next_child_time_nano += 500000000;
        if(next_child_time_nano >= NANOSECOND)
        {
                next_child_time_sec += 1;
                next_child_time_nano = 0;
        }
	
	if((msgrcv(msgqueueId,  &msgqueue, sizeof(msgqueue), 1, IPC_NOWAIT)) != -1)
	{ 
	if(msgqueue.request_type == 0)
	{
		int processId = msgqueue.processNumber;
		int index = getIndexById(processId);
		int resource = msgqueue.resourceId;
		//if(system_state -> available[resource] > 0)
		system_state -> allocated[index][resource] += 1;
		system_state -> available[resource] -= 1;
		fprintf(stderr, "Request %d , by process %d \n", resource, processId);
		if(checkIfSafeState(*system_state))
		{
			msgqueue.msg_type = processId;
			fprintf(stderr, "Safe State:  Request %d, by process %d \n", resource, processId); 
			printInfo();
			msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
		else
		{
			fprintf(stderr, "********************************* \n");
			fprintf(stderr, "Detected Unsafe State: Request denied for Process %d, Resource %d\n", processId, resource);
			system_state -> allocated[index][resource] -= 1;
                	system_state -> available[resource] += 1;		
			//enqueue(resource, resource);
			//enqueue(process, processId);
		}
	}
	else if(msgqueue.request_type == 1)
	{
		int processId = msgqueue.processNumber;
                int index = getIndexById(processId);
                int resource = msgqueue.resourceId;
		system_state -> allocated[index][resource] -= 1;
                system_state -> available[resource] += 1;	
	} 
	}

	clock -> nanoseconds += 1000000;
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
	}
}

wait(NULL);
clearSharedMemory();
return 0;
}
