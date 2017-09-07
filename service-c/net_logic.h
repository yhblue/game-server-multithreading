#ifndef _NET_LOGIC_
#define _NET_LOGIC_

#include "lock_queue.h"
#include "configure.h"

#define MESSAGE_QUEUE_NUM    7

#define QUE_ID_NETIO_2_NETLOGIC          0
#define QUE_ID_NETLOGIC_2_NETIO          1
#define QUE_ID_NETLOGIC_2_GAME_FIRST     2
#define QUE_ID_NETLOGIC_2_GAME_SECOND	 3
#define QUE_ID_NETLOGIC_2_GAME_THIRD	 4
#define QUE_ID_NETLOGIC_2_GAME_FOURTH	 5
#define QUE_ID_GAMELOGIC_2_NETLOGIC	     6  

#define SERVICE_ID_NETWORK_IO       	 0
#define SERVICE_ID_NET_ROUTE        	 1
#define SERVICE_ID_LOG              	 2
#define SERVICE_ID_DB               	 3
#define SERVICE_ID_GAME_FIRST       	 4
#define SERVICE_ID_GAME_SECOND      	 5
#define SERVICE_ID_GAME_THIRD       	 6
#define SERVICE_ID_GAME_FOURTH      	 7

#define MAX_SERVICE      			8

#define GAME_LOGIC_SERVICE_NUM      4
#define GAME_LOGIC_SERVER_FIRST     0
#define GAME_LOGIC_SERVER_SECOND    1
#define GAME_LOGIC_SERVER_THIRD     2
#define GAME_LOGIC_SERVER_FOURTH    3


typedef struct _net_logic_start
{
	queue* que_pool;
	int service_id;	

	char* netio_addr;
	int netio_port;
	
	char* service_addr;
	int service_port;
}net_logic_start;




queue* message_que_creat();
net_logic_start* net_logic_start_creat(queue* que_pool,configure* conf,int service_id);
void* net_logic_service_loop(void* arg);
#endif
