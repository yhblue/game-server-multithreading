#include "socket_server.h"
#include "net_logic.h"
#include "configure.h"
#include "game_logic.h"

#include <pthread.h>
  
int main()
{
	pthread_t pthread_id[8];

	queue* que_pool = message_que_creat();

	//网络IO线程
	net_io_start* netio_start = net_io_start_creat(que_pool,0,"127.0.0.1",8000);
	pthread_create(&pthread_id[0],NULL,network_io_service_loop,netio_start);  

	//网络路由线程
	net_logic_start* netlog_start = net_logic_start_creat(que_pool);
	pthread_create(&pthread_id[1],NULL,net_logic_service_loop,netlog_start); 

	game_router* route = NULL;	
	game_logic_start* game_start01 = game_logic_start_creat(que_pool,route,SERVICE_ID_GAME_FIRST,"127.0.0.1",8001,8002);
	game_logic_start* game_start02 = game_logic_start_creat(que_pool,route,SERVICE_ID_GAME_SECOND,"127.0.0.1",8001,8003);
	game_logic_start* game_start03 = game_logic_start_creat(que_pool,route,SERVICE_ID_GAME_THIRD,"127.0.0.1",8001,8004);
	game_logic_start* game_start04 = game_logic_start_creat(que_pool,route,SERVICE_ID_GAME_FOURTH,"127.0.0.1",8001,8005);

	pthread_create(&pthread_id[2],NULL,game_logic_service_loop,game_start01);
	pthread_create(&pthread_id[3],NULL,game_logic_service_loop,game_start02);
	pthread_create(&pthread_id[4],NULL,game_logic_service_loop,game_start03);
	pthread_create(&pthread_id[5],NULL,game_logic_service_loop,game_start04);

	pthread_join(pthread_id[0],NULL);		//等待线程结束
	pthread_join(pthread_id[1],NULL);
	pthread_join(pthread_id[2],NULL);		//等待线程结束
	pthread_join(pthread_id[3],NULL);
	pthread_join(pthread_id[4],NULL);		//等待线程结束
	pthread_join(pthread_id[5],NULL);	
	return 0;
}























