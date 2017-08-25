#ifndef _GAME_LOGIC_H
#define _GAME_LOGIC_H


typedef struct _game_logic_start
{
	queue* que_2_net_logic;
	int service_id;	
	int service_port;
	char* serv_addr;
	game_router* route;
}game_logic_start;



#endif



