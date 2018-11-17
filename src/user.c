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

int status,x,i,m,n,p,q;
int clockId, msgqueueId,msgqueueKey, stateId;
logical_clock *clock;
res_desc *system_state;
pid_t mypid;
int pid_maxclaims[20], pid_alloc[20];
long int startsec, startnano;
long seed_child = 0;


int main (int argc, char *argv[], char *res_claims[]) {

seed_child = time(NULL);
mypid = getpid();
clockId = atoi(argv[0]);
msgqueueId = atoi(argv[1]);
stateId = atoi(argv[2]);

int t = 0;
for(t = 0; t< 20; t++)
	pid_maxclaims[t] = atoi(res_claims[t]);

clock = (logical_clock*) shmat(clockId, NULL, 0);

if(clock == (void *) -1)
{
        perror("Error in attaching Clock User \n");
        exit(1);
}

system_state = (res_desc*) shmat(stateId, NULL, 0);
if(system_state == (void *)-1)
{
	perror("Error in State shmat User \n");
	exit(1);
}

int m ;
for(m = 0; m < 20; m++)
	pid_alloc[m] = 0;

int randomNumber, choice;
while(1)
{
	if((clock -> seconds >= startsec) || (clock -> nanoseconds >= startnano))
	{	
	srand((seed_child++)  * mypid);
	randomNumber = rand() % 100;
	if(randomNumber <= 50)
        	choice = 0;
	else if(randomNumber <= 90)
        	choice = 1;
	else
		choice = 2;

	if(choice == 0)		
	{
		srand((seed_child++)  * mypid);
		int resource = rand() % 20;
		if(pid_alloc[resource] < pid_maxclaims[resource])
		{
		msgqueue.msg_type = 1;
		msgqueue.processNumber = mypid;
		msgqueue.resourceId = resource;
		msgqueue.request_type = 0;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		msgrcv(msgqueueId, &msgqueue, sizeof(msgqueue), mypid, 0);
		pid_alloc[resource]++;
		}	
	}
	else if(choice == 1)  
	{
                srand((seed_child++)  * mypid);
		int resource = rand() % 20;
		if(pid_alloc[resource] > 0)
		{
		msgqueue.msg_type = 1;
		msgqueue.processNumber = mypid;
		msgqueue.resourceId = resource;
		msgqueue.request_type = 1;
		pid_alloc[resource]--;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
	}
	else
	{
		msgqueue.msg_type = 1;
		msgqueue.request_type = 2;
		msgqueue.processNumber = mypid;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		break;
	} 
	
	srand((seed_child++)  * mypid);
	startnano += rand() % 1000;
	if(startnano >= NANOSECOND)
	{
		startsec += startnano/NANOSECOND;
		startnano = startnano % NANOSECOND;
	}
	}
} 

return 0;
}
