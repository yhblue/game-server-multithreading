#ifndef _NET_LOGIC_
#define _NET_LOGIC_

#include "lock_queue.h"
#include "configure.h"

#define MESSAGE_QUEUE_NUM    8

#define QUE_ID_NETIO_2_NETLOGIC          0
#define QUE_ID_NETLOGIC_2_NETIO          1
#define QUE_ID_NETLOGIC_2_GAME_FIRST     2
#define QUE_ID_NETLOGIC_2_GAME_SECOND	 3
#define QUE_ID_NETLOGIC_2_GAME_THIRD	 4
#define QUE_ID_NETLOGIC_2_GAME_FOURTH	 5
#define QUE_ID_GAMELOGIC_2_NETLOGIC	     6 
#define QUE_ID_LOG_SERVER				 7 

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

#define INVALID 				   -1


//一个游戏逻辑线程最大支持20*10个玩家在线
#define MAX_MAP    						    20
#define MAX_PLAYER_EACH_MAP  			    10
#define MAX_GAME_SERVICE_PLAYER_NUM	   20 * 10


//
#define TYPE_EVENT_SERVICE_FULL		'F'
#define TYPE_EVENT_PLAYER_ERROR		'E'

typedef struct _net_logic_start
{
	queue* que_pool;
	int service_id;	

	char* netio_addr;
	int netio_port;
	
	char* service_addr;
	int service_port;
}net_logic_start;

//网关服务中网络IO处理线程与事务处理线程通信的数据类型约定
//消息队列的双方的通信约定
#define TYPE_DATA     'D'   //普通数据包
#define TYPE_CLOSE    'C'   //客户端关闭
#define TYPE_SUCCESS  'S'   //新客户端完全登陆成功


typedef struct _msg_head
{
    char msg_type;           //'D' 'S' 'C'
    char proto_type;         //for client data ,is serilia type,for inform is 
    int uid;                 //socket uid
    int len;                 //for data is buffer length,for other is 0 
}msg_head;


queue* message_que_creat();
msg_head* msg_head_create(char msg_type,char proto_type,int uid,int len);
net_logic_start* net_logic_start_creat(queue* que_pool,configure* conf,int service_id);
void* net_logic_service_loop(void* arg);
#endif
