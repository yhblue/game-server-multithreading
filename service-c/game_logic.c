#include "game_logic.h"
#include "socket_server.h"
#include "message.pb-c.h"
#include "net_logic.h"
#include "err.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
//http://www.cnblogs.com/purpleraintear/p/6160733.html

//一个游戏逻辑线程最大支持20*10个玩家在线
#define MAX_MAP    				20
#define MAX_PLAYER_EACH_MAP  	10


typedef struct _player_msg
{
	int uid;
}player_msg;

typedef struct _position
{
	int point_x;
	int point_y;
}position;

typedef struct _technique
{
	int speed;
}technique;

typedef struct _player
{
	player_msg msg;
	position pos;
	technique tec;
}player;

typedef struct _map_msg
{
	int length;
	int width;
}map_msg;

//id和map_player怎么对应上
typedef struct _game_logic
{
	int player_num;               	 //这个进程里面的游戏总人数
	int map_player_num[MAX_MAP];  	 //每个地图里面的玩家数目
	int sock_2_net_logic;         	 //通信的socket
	queue* que_2_net_logic;		  	 //通信的消息队列
	player* map_player[MAX_MAP];  	 //map_player 是一个指向 sizeof(player) * MAX_PLAYER_EACH_MAP 的头指针
	game_router* route;				 //路由表
	char* netlog_addr;
	int netlog_port;
	int service_port;
	fd_set select_set;
	bool check_que;
}game_logic;


game_router* game_route_table_creat()
{
	game_router* route = (game_router*)malloc(sizeof(game_router)*MAX_SOCKET);
	if(route == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:route malloc failed\n");
		return NULL;		
	}
	return route;
}

game_logic_start* game_logic_start_creat(queue* que_pool,game_router* route,int service_id,char* netlog_addr,int netlog_port,int serv_port)
{
	game_logic_start* start = (game_logic_start*)malloc(sizeof(game_logic_start));
	start->que_2_net_logic = &que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
	start->service_id = service_id;
	start->service_port = serv_port;
	start->netlog_addr = netlog_addr;
	start->netlog_port = netlog_port;
	start->route = route;

	return start;
}

static game_logic* game_logic_creat(game_logic_start* start)
{
	game_logic* gl = (game_logic*)malloc(sizeof(game_logic));

	for(int i=0; i<MAX_MAP; i++)
	{
		gl->map_player[i] = (player*)malloc(sizeof(player)*MAX_PLAYER_EACH_MAP);
	}
	for(int i=0; i<MAX_MAP; i++)
	{
		gl->map_player_num[i] = 0;
	}
	gl->route = start->route;
	gl->netlog_addr = start->netlog_addr;
	gl->netlog_port = start->netlog_port;
	gl->service_port = start->service_port;
	gl->check_que = false;

	FD_ZERO(&gl->select_set); 
}

static bool have_event(game_logic* gl)
{
	
    FD_SET(gl->sock_2_net_logic,&gl->select_set);
//    struct timeval tv = {0,0};
    int ret = select(gl->sock_2_net_logic+1,&gl->select_set,NULL,NULL,NULL);    
	if (ret == 1) 
	{
		return true;
	}
	return false;    
}

static int dispose_queue_event(game_logic* gl)
{
	return 0;
}

static int dispose_socket_event(game_logic* gl)
{
	char buf[64];
	int n = read(gl->sock_2_net_logic,buf,sizeof(buf));
	if(n < 0)
	{
		switch(errno)
		{ 
			case EINTR:
			case EAGAIN:
				fprintf(ERR_FILE,"dispose_socket_event: errno\n");
				return 0;    	
			default:
				close(gl->sock_2_net_logic);
				fprintf(ERR_FILE,"dispose_socket_event: game_logic read err,close socket\n");
				return -1;    		
		}
	}
	if(n == 0) //net logic close socket
	{
		close(gl->sock_2_net_logic);
		fprintf(ERR_FILE,"dispose_socket_event: net_logic service close socket\n");
		return -1;
	}	
	return 0;
}

static int game_logic_event(game_logic* gl)
{
	int ret = 0;
	for( ; ; )
	{
		if(gl->check_que == true)
		{
			ret = dispose_queue_event(gl);
			if(ret == -1) //queue is null
			{
				gl->check_que = false;
				return -1;
			}
			else
			{
				continue;
			}
		}
		if(have_event(gl) == true)
		{
			gl->check_que = true;
			dispose_socket_event(gl);
		}
	}
}

void* game_logic_service_loop(void* arg)
{
	game_logic_start* start = (game_logic_start*)arg;
	game_logic* gl = game_logic_creat(start);
}

