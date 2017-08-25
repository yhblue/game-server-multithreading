#ifndef _NET_LOGIC_
#define _NET_LOGIC_

#include "lock_queue.h"

#define MESSAGE_QUEUE_NUM    7

#define QUE_ID_NETIO_2_NETLOGIC          0
#define QUE_ID_NETLOGIC_2_NETIO          1
#define QUE_ID_NETLOGIC_2_GAME_FIRST     2
#define QUE_ID_NETLOGIC_2_GAME_SECOND	 3
#define QUE_ID_NETLOGIC_2_GAME_THIRD	 4
#define QUE_ID_NETLOGIC_2_GAME_FOURTH	 5
#define QUE_ID_GAMELOGIC_2_NETLOGIC	     6  

// typedef struct _double_que
// {
// 	queue* que_to;
// 	queue* que_from;
// }double_que;

typedef struct _net_logic_start
{
	queue* que_pool;
	int service_id;	
	char* netio_addr;
	int netio_port;
}net_logic_start;


queue* communication_que_creat();
net_logic_start* net_logic_start_creat(queue* que_pool);
void* net_logic_service_loop(void* arg);
#endif
