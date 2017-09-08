#ifndef _GAME_LOGIC_H
#define _GAME_LOGIC_H

#include "lock_queue.h"
#include "net_logic.h"
#include <stdint.h>

//一个游戏逻辑线程最大支持20*10个玩家在线
#define MAX_MAP    						    20
#define MAX_PLAYER_EACH_MAP  			    10
#define MAX_GAME_SERVICE_PLAYER_NUM	   20 * 10

typedef struct _player_id
{
	char state;						//记录这个成员空间的状态
	uint8_t mapid;     		    	//记录这个用户在哪一个地图上
	uint8_t map_playerid;  			//记录这个用户在mapid上的这个地图的玩家的id 0-9
}player_id;

//我觉得可以在这里加一个判断位 true 才将UID列表更新过去 否则网络IO线程就使用原来的发送
typedef struct _broadcast_list
{
	int broadcast_player_num;			//要广播的玩家数目
	int uid_list[MAX_PLAYER_EACH_MAP];	//要广播的 uid 列表	
}broadcast_list;


typedef struct _broadcast_data
{
	char proto_type;				//要打包的类型
	void* buffer;					//要打包的数据
}broadcast_data;

typedef  broadcast_list  game_msg_head;
typedef  broadcast_data  game_msg_data;

typedef struct _broadcast
{
	broadcast_list list;			//要广播的成员列表
	broadcast_data data;			//要发送的数据
}broadcast;

//转换成队列节点就是 broadcast_list 存放头部
//					 broadcast_data 存放在数据部分


//网络逻辑处理服务应该只是更换掉broadcast数据类型的data中的void的指向
//网络IO线程在发送完数据给最后一个玩家后要free(broadcast->data->buffer)
//										  free(broadcast)
//										  
typedef struct _game_router
{
	int player_num;               	 		 //这个进程里面的游戏总人数
	int map_player_num[MAX_MAP];  	 		 //每个地图里面的玩家数目
	player_id* uid_2_playid;		 		 //MAX_SOCKET个成员,uid->player_id的映射
	int* map_player_socket_uid[MAX_MAP];	 //存储这个地图中的用户 uid MAX_PLAYER_EACH_MAP;
									 		 //map_player_uid[map_id]存储是map_id地图中的MAX_PLAYER_EACH_MAP个成员的UID
					 						 //如果对应成员不存在则置空		
	
	int *map_uid_list[MAX_MAP];			 	 //为每一个地图维护一个用于广播的 map_uid_list,地图内有效的玩家uid永远位于最前面
}game_router;								 //这个广播列表应该放在网络IO线程吗?

typedef struct _game_logic_start
{
	queue* que_2_net_logic;
	queue* que_pool;
	int service_id;	
	int service_port;
	char* service_route_addr; 	//路由服务的addr
	int service_route_port;   	//路由服务的port
	player_id* uid_2_playid;
	int game_service_id;

}game_logic_start;

player_id* playerid_list_creat(void);
game_logic_start* game_logic_start_creat(queue* que_pool,configure* conf,player_id* uid_2_playid,int service_id);
void* game_logic_service_loop(void* arg);

#endif



