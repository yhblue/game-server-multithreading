#include "socket_server.h"
#include "net_logic.h"
#include "configure.h"
#include "game_logic.h"
#include "configure.h"

#include <pthread.h>
  
int main()
{
	pthread_t pthread_id[6] = {0};

	configure* conf = configure_load(conf_file);
	queue* que_pool = message_que_creat();

	//网络IO线程
	net_io_start* netio_start = net_io_start_creat(que_pool,conf,SERVICE_ID_NETWORK_IO);
	pthread_create(&pthread_id[0],NULL,network_io_service_loop,netio_start);  

	//网络路由线程
	net_logic_start* netlog_start = net_logic_start_creat(que_pool,conf,SERVICE_ID_NET_ROUTE);
	pthread_create(&pthread_id[1],NULL,net_logic_service_loop,netlog_start); 

	player_id* uid_2_playerid = playerid_list_creat();
	if(uid_2_playerid == NULL) 
		return -1;

	//游戏逻辑服
	game_logic_start* game_start01 = game_logic_start_creat(que_pool,conf,uid_2_playerid,SERVICE_ID_GAME_FIRST);
	game_logic_start* game_start02 = game_logic_start_creat(que_pool,conf,uid_2_playerid,SERVICE_ID_GAME_SECOND);
	game_logic_start* game_start03 = game_logic_start_creat(que_pool,conf,uid_2_playerid,SERVICE_ID_GAME_THIRD);
	game_logic_start* game_start04 = game_logic_start_creat(que_pool,conf,uid_2_playerid,SERVICE_ID_GAME_FOURTH);

	pthread_create(&pthread_id[2],NULL,game_logic_service_loop,game_start01);
	pthread_create(&pthread_id[3],NULL,game_logic_service_loop,game_start02);
	pthread_create(&pthread_id[4],NULL,game_logic_service_loop,game_start03);
	pthread_create(&pthread_id[5],NULL,game_logic_service_loop,game_start04);

	pthread_join(pthread_id[0],NULL);		
	pthread_join(pthread_id[1],NULL);
	pthread_join(pthread_id[2],NULL);		
	pthread_join(pthread_id[3],NULL);
	pthread_join(pthread_id[4],NULL);		
	pthread_join(pthread_id[5],NULL);	

	return 0;
}























