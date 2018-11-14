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
	int processNumber;
	int resourceId;
	int request_type; //0 for request and 1 for release
}msgqueue;


#endif
