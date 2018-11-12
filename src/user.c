#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <signal.h>
#include "shared_mem.h"
#include <fcntl.h>
#define MILLION 1000000
#define NANOSECOND 1000000000
#include <semaphore.h>

shared_mem *clock;
pcb *pcbs;
resource *resources;
req_res *request;
int shmId, pcbshmId, resId, req_resId;
sem_t *mySemaphore;
char *semName;
pid_t mypid;
int x;

int randomNumberGenerator(int min, int max)
{
	return ((rand() % (max-min +1)) + min);
}

int getPCBBlockId(int processId)
{
	int q;
        for(q = 0; q<18;q++)
        {
                if(pcbs[q].processId == processId)
                return q;
        }
}

int main (int argc, char *argv[]) {

mypid = getpid();
while((x = getopt(argc,argv, "p:s:j:k:b:")) != -1)
switch(x)
{
case 'p': 
	resId = atoi(optarg);
	break;
case 's':
        shmId = atoi(optarg);
        break;
case 'j': 
	pcbshmId = atoi(optarg);
	break;
case 'k':
	semName = optarg;
	break;
case 'b':
	req_resId = atoi(optarg);
	break;
case '?':
        fprintf(stderr, "Invalid Arguments in user\n");
        return 1;
}

clock = (shared_mem*) shmat(shmId, NULL, 0);

if(clock == (void*) -1)
{
        perror("Error in attaching shared memory User \n");
        exit(1);
}

pcbs = (pcb*) shmat(pcbshmId, NULL, 0);
if(pcbs == (void*) -1)
{
	perror("Error in attaching shared memory to PCB Array \n");
	exit(1);
}

resources = (resource*) shmat(resId, NULL, 0);
if(resources == (void*) -1)
{
	perror("error in attaching memory to resources \n");
	exit(1);
}

request = (req_res*) shmat(req_resId, NULL, 0);
if(request == (void*) -1)
{
	perror("Error in attaching shared memory to request \n");
        exit(1);	
}

/* int j;

for(j =0; j < 20; j++)
{
	fprintf(stderr, "resources value: %d --  %d \n", j, resources[j].total_instances);
} */

int randomNumber, choice;
int mypid = getpid();
/* while(1)
{
	randomNumber = randomNumberGenerator(0,100);
	if(randomNumber <= 10)
        	choice = 0;
	else if(randomNumber <= 75)
        	choice = 1;
	else
        	choice = 2;
	
	if(choice == 1)
	{
		request -> processNumber = mypid; 
		int m = 0;
		srand(m++);
		int resource = rand() % 20;
		request -> resourceId = resource;
		int blockId = getPCBBlockId(mypid);
		pcbs[blockId].request = resource;
		//sem_wait(mySemaphore);
			
	}
	else if(choice == 2)	
	{
		int blockId = getPCBBlockId(mypid);
		int n;
		for(n = 0; n < 20; n++)
		{
			if(pcbs[blockId].allocation.qty[n] > 0)
			{
			pcbs[blockId].release = n;
			break;
			}
		}
	}	
} */

return 0;
}
