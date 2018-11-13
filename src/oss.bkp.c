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
//#include "priority_queue.h"
#define NANOSECOND 1000000000
#define LOGFILESIZE 100000
int status,x,i,m,n,p,q;
key_t clockKey,shmKey, stateKey;
int clockId, pcbId, stateId, shmId;
logical_clock *clock;
shared_mem *sharedmem;
res_desc *state;
pcb *pcbs;
FILE *logfile;
char *file_name;


void clearSharedMemory() {
fprintf(stderr, "Total Children Forked : %d \n", child_count);
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
shmdt((void *)pcbs);
shmdt((void *)sharedMem);
shmdt((void *)state);
//fprintf(stderr,"Closing File \n");
//fclose(logfile);
fprintf(stderr, "OSS started detaching all the shared memory segments \n");
shmctl(clockId, IPC_RMID, NULL);
shmctl(pcbId, IPC_RMID, NULL);
shmctl(stateId, IPC_RMID, NULL);
shmctl(shmId, IPC_RMID, NULL);
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}

void killExistingChildren(){
int k;
for(k=0; k<18; k++)
{
if(pcbs[k].processId != 0)
{
kill(pcbs[k].processId, SIGTERM);
}
}
}

void myhandler(int s) {
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

//Queue* blocked_queue = create_queue(20);

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

int pcbshmSize = sizeof(pcbs) * 18;
pcbKey = ftok(".", 'x');
pcbId = shmget(pcbKey, pcbshmSize, IPC_CREAT | 0666);
if(pcbId < 0)
{
	fprintf(stderr, "Error in Shmget for PCB's \n");
	exit(1);
}

pcbs = (pcb*) shmat(pcbId, NULL, 0);
if(pcbs == (void *) -1)
{
	perror("Error in attaching Memory to process_control_blocks \n");
	exit(1);
}

stateKey = ftok(".bac", 's');
stateId = shmget(stateKey, sizeof(res_desc), IPC_CREAT | 0666);
if(stateId < 0)
{
	fprintf(stderr, "Error in Shmget for State \n");
	exit(1);
}

state = (res_desc*) shmat(stateId, NULL, 0);
if(state == (void *) -1)
{
	perror("Error in attaching Memory to State\n");
	exit(1);
}

shmKey = ftok(".", 'c');
shmId = shmget(shmKey, sizeof(shared_mem), IPC_CREAT | 0666);

if(shmId <0 )
{
	fprintf(stderr, "Error in shmget \n");
	exit(1);
}

sharedmem = (shared_mem*) shmat(shmId, NULL, 0);




wait(NULL);
return 0;
}
