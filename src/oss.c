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
key_t clockKey, stateKey, msgqueueKey;
int clockId, stateId,  msgqueueId;
logical_clock *clock;
res_desc *system_state;
FILE *logfile;
char *file_name;
char *res_claims[21];
int child_pids[18], term = 0, child_count = 0;
long seed_value ;
int bit_vector[18], deadlock_run = 0, log_lines = 0, verbose = 1;
Queue* process;
Queue* resources;
int count_release = 0, count_blocked = 0, count_unb_safe = 0;
int updated_pids[18], last_used = 0;
int updated_res[18];

void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)system_state);
fprintf(stderr,"Closing File \n");
fclose(logfile);
fprintf(stderr, "DeadLock ran %d \n", deadlock_run);
fprintf(stderr, "Acknowledged %d \n", count_release);
fprintf(stderr, "Blocked: %d \n", count_blocked);
fprintf(stderr, "Terminated %d \n", term);
fprintf(stderr, "Child Forked %d \n", child_count); 
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
	if(log_lines < 100000 && verbose)
	{
	log_lines++;
	fprintf(logfile, "Resources : ");
	}
	for (i = 0; i < 20; i++)
        {
		if(log_lines < 100000 && verbose)
	        {
		log_lines++;
		fprintf(logfile, "%d  ", system_state -> resources[i]);	
		}
	}
	if(log_lines < 100000 && verbose)
        {
	log_lines++;
	fprintf(logfile, "\n");
	fprintf(logfile, "Available : ");
	}
        for (i = 0; i < 20; i++)
        {
		if(log_lines < 100000 && verbose)
	        {
		log_lines++;
		fprintf(logfile, "%d  ", system_state -> available[i]);
		}
        }
	 if(log_lines < 100000 && verbose)
        {
	log_lines++;
	fprintf(logfile, "\n");
	}

for (p =0; p< 18;p++)
{
        for(q=0; q< 20; q++)
        {
	 if(log_lines < 100000 && verbose)
        {
	log_lines++;
        fprintf(logfile, "%d ", system_state -> allocated[p][q]);
	}
	//fprintf(stderr, "%d ", system_state -> available[q]);
        if(q == 19)
	 if(log_lines < 100000 && verbose)
        {
	log_lines++;
        fprintf(logfile, "\n");
        }
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
	deadlock_run++;
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

void run_blocked_process()
{
	last_used = 0;
	while(isEmpty(process) > 0)
	{
	int processId = dequeue(process);
	int resource = dequeue(resources);
	int index = getIndexById(processId);
	if(system_state -> available[resource] > 0)
        {
	int index = getIndexById(processId);
	bit_vector[index] = 1;
	if(log_lines < 100000 && !verbose)
        {
        log_lines++;
	fprintf(logfile, "Master Unblocking process %d requesting  %d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
	}
	system_state -> allocated[index][resource] += 1;
	system_state -> available[resource] -= 1;
	if(checkIfSafeState(*system_state))
	{
		count_unb_safe++;	
		if(log_lines < 100000 && !verbose)
                {
                log_lines++;
		fprintf(logfile, "Master granted process %d requesting  %d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
		printInfo();
		fprintf(logfile, "Master removed process %d  from blocked queue %d \n",processId, resource); 
		}
	}
	else
	{
		 if(log_lines < 100000 && !verbose)
                {
                log_lines++;
		fprintf(logfile, "Master has detected unsafe state for process %d requesting  %d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
		}
		system_state -> allocated[index][resource] -= 1;
                system_state -> available[resource] += 1;
		updated_pids[last_used] = processId;
		updated_res[last_used] = resource;
		last_used++;
	}
	}
	else
	{
		updated_pids[last_used] = processId;
                updated_res[last_used] = resource;
		if(log_lines < 100000 && !verbose)
                {
                log_lines++;
		fprintf(logfile, "Master cannot allocate the request for unblockd process %d for request %d at time %ld:%ld\n",  processId, resource, clock -> seconds, clock -> nanoseconds);
		}
       	     last_used++;	
	}
	}
	int j;
	for(j = 0; j < last_used; j++)
	{
		enqueue(process, updated_pids[j]);
		enqueue(resources, updated_res[j]);
	}
}

void initialize_resources(){
int i;
	for(i = 0; i < 20; i++)
	{
		srand(seed_value++);
		system_state -> resources[i] = rand() % (20 - 10 + 1) + 10;
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
		system_state -> maxclaims[pindex][q] = rand() % 10 + 1;
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
case 'v':
	verbose = atoi(optarg);
	break;
case '?':
        fprintf(stderr, "Please give '-h' for help to see valid arguments \n");
        return 1;
}

process = create_queue(20);
resources = create_queue(20);

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
if(file_name == NULL)
file_name = "default";
logfile = fopen(file_name, "w");

fprintf(stderr, "Opened Log File for writing Output::: %s \n", file_name);


pid_t mypid;
int n;
for(n = 0;n < 18; n++)
{
	child_pids[n] = 0;
	bit_vector[n] = -1;
}

initialize_resources();


int index = -1,b, next_child_time_nano, next_child_time_sec;
while(1)
{
	//perror("OSS ");
	index = -1;
	for(b = 0; b< 18; b++)
	{
		if(child_pids[b] == 0)
		{
		index = b;
		break;
		}
	}
		if((index >= 0) && (next_child_time_sec >= clock -> seconds || next_child_time_nano >= clock -> nanoseconds))
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
		child_count++;
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
		if(system_state -> available[resource] > 0)
		{
		system_state -> allocated[index][resource] += 1;
		system_state -> available[resource] -= 1;
		if(log_lines < 100000 && verbose)
		{
		log_lines++;
		fprintf(logfile, "Master has detected process %d requesting  R%d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
		}
		if(checkIfSafeState(*system_state))
		{
			msgqueue.msg_type = processId;
			if(log_lines < 100000 && verbose)
                	{
                	log_lines++;
			fprintf(logfile, "Master granting process  %d request  R%d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
			printInfo();
			}
			msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
		else
		{
			if(log_lines < 100000 && verbose)
                	{
                	log_lines++;
			fprintf(logfile, "************************************************************************************************ \n");
			fprintf(logfile, "Detected Unsafe State: Request denied for Process %d, Resource %d\n", processId, resource);
			}
			system_state -> allocated[index][resource] -= 1;
                	system_state -> available[resource] += 1;		
			enqueue(resources, resource);
			enqueue(process, processId);
			count_blocked++;
			bit_vector[index] = 0;
			if(log_lines < 100000 && !verbose)
                	{
                	log_lines++;
			fprintf(logfile, "Master blocking process %d requesting  %d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
			}
		}
		}
		else
		{
			if(log_lines < 100000 && verbose)
                	{
                	log_lines++;
			fprintf(logfile, "Request > AVailable. Suspending process %d at time %ls:%ld \n", processId,  clock -> seconds, clock -> nanoseconds);
			}
			enqueue(resources, resource);
                        enqueue(process, processId);
		}
	}
	else if(msgqueue.request_type == 1)
	{
		int processId = msgqueue.processNumber;
                int index = getIndexById(processId);
                int resource = msgqueue.resourceId;
		system_state -> allocated[index][resource] -= 1;
                system_state -> available[resource] += 1;
		count_release++;	
		if(log_lines < 100000 && verbose)
                {
                log_lines++;
		fprintf(logfile, "Master has acknowledged %d releasing  %d at time %ld:%ld \n",  processId, resource, clock -> seconds, clock -> nanoseconds);
		}
		run_blocked_process();
	}
	else 
	{
		int processId = msgqueue.processNumber;
		term++;
		if(log_lines < 100000 && verbose)
                {
                log_lines++;
		fprintf(logfile, "Master found process  %d terminating, releasing all its allocated at time %ld:%ld \n",  processId, clock -> seconds, clock -> nanoseconds);
                }
		int index = getIndexById(processId);
		int n;
		for(n = 0; n < 20; n++)
		{
		system_state -> available[n] += system_state -> allocated[index][n];
		system_state -> allocated[index][n] = 0;
		}
		bit_vector[index] = -1;
                child_pids[index] = 0;
		waitpid(processId, &status, 0);
	} 
	}
	else
		clock -> seconds += 1;

	clock -> nanoseconds += 1000;
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
