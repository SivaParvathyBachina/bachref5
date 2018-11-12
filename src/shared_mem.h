#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

typedef struct
{
	long seconds;
	long nanoseconds;
}shared_mem;

typedef struct 
{
	int qty[20];
}res_desc;

typedef struct
{
	pid_t processId;
	int request;
	int release;
	int terminate; 
	res_desc maxclaim;
	res_desc allocation;
	
}pcb;

typedef struct
{
	int total_instances;
	int avail_instances;
}resource;

typedef struct
{
	int processNumber;
	int resourceId;
}req_res;

#endif
