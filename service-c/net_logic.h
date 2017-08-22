#ifndef _NET_LOGIC_
#define _NET_LOGIC_

#include "lock_queue.h"

typedef struct _double_que
{
	queue* que_to;
	queue* que_from;
}double_que;

typedef struct _net_logic_start
{
	double_que* que_pool;
	int thread_id;	
	char* netio_addr;
	int netio_port;
}net_logic_start;


double_que* communication_que_creat();
net_logic_start* net_logic_start_creat(double_que* que_pool);
void* net_logic_service_loop(void* arg);
#endif
