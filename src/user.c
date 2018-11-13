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
int clockId, msgqueueId,msgqueueKey, stateId, shmId;
logical_clock *clock;
shared_mem *sharedmem;
res_desc *system_state;
pid_t mypid;
long int startsec, startnano;


int main (int argc, char *argv[], char *res_claims[]) {

mypid = getpid();
clockId = atoi(argv[0]);
shmId = atoi(argv[1]);
msgqueueId = atoi(argv[2]);
stateId = atoi(argv[3]);

//int t = 0,r;
//for(t = 0; t< 20; t++)
//	fprintf(stderr,"%d \t",atoi(res_claims[t]));

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


//msgqueueKey = ftok(".bacmsg", 'b');
//msgqueueId = msgget(msgqueueKey, IPC_CREAT | 0666);
msgrcv(msgqueueId, &msgqueue, sizeof(msgqueue), 1, 0);
//fprintf(stderr, "Message received is %s \n", msgqueue.msg_txt);


int randomNumber;
/*while(1)
{
	if((clock -> seconds >= startsec) && (clock -> nanoseconds >= startnano))
	{	
	randomNumber = randomNumberGenerator(0,100);
	if(randomNumber <= 50)
        	choice = 0;
	else if(randomNumber <= 98)
        	choice = 1;
	else
		choice = 2;

	if(choice == 0)		//Request a resource
	{
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 1);
		msgrcv(msgqueueId, &msgqueue, size(msgqueue), mypid, 0);
	}
	else if(choice == 2)   //Release a resource
	{
	}
	else
	{
		//break;
	} 
	
	startsec += 1;
	startnano += rand() % 1000;
	}
} */

return 0;
}
