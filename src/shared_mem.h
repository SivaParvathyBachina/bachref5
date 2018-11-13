#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

typedef struct
{
	long seconds;
	long nanoseconds;
}logical_clock;


typedef struct
{
	int resources[20];
	int available[20];
	int maxclaims[18][20];
	int allocated[18][20];
}res_desc;

struct msg_buf{
	long msg_type;
	char msg_txt[100];
}msgqueue;

typedef struct
{
	int processNumber;
	int resourceId;
	int instances;
	int granted;
}shared_mem;

#endif
