#ifndef _GAME_LOGIC_H
#define _GAME_LOGIC_H

#include "lock_queue.h"

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

typedef struct _game_router
{
	int player_num;               	 //这个进程里面的游戏总人数
	int map_player_num[MAX_MAP];  	 //每个地图里面的玩家数目
	player_id* uid_2_playid;		 //MAX_SOCKET个成员
}game_router;

typedef struct _game_logic_start
{
	queue* que_2_net_logic;
	queue* que_pool;
	int service_id;	
	int service_port;
	char* netlog_addr;
	int netlog_port;
	game_router* route;
	int game_service_id;
}game_logic_start;

game_logic_start* game_logic_start_creat(queue* que_pool,game_router* route,int service_id,char* netlog_addr,int netlog_port,int serv_port,int game_service_id);

void* game_logic_service_loop(void* arg);

#endif



