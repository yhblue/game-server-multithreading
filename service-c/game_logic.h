#ifndef _GAME_LOGIC_H
#define _GAME_LOGIC_H

#include "lock_queue.h"

#include <stdint.h>

typedef struct _player_id
{
	uint8_t mapid;     		    	//记录这个用户在哪一个地图上
	uint8_t map_playerid;  			//记录这个用户在mapid上的这个地图的玩家的id 0-9
}player_id;

typedef struct _game_router
{
	player_id* uid_2_playid;		//MAX_SOCKET个成员
}game_router;

typedef struct _game_logic_start
{
	queue* que_2_net_logic;
	int service_id;	
	int service_port;
	char* netlog_addr;
	int netlog_port;
	game_router* route;
}game_logic_start;

game_logic_start* game_logic_start_creat(queue* que_pool,game_router* route,int service_id,char* netlog_addr,int netlog_port,int serv_port);

void* game_logic_service_loop(void* arg);

#endif



