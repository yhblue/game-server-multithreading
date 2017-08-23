#include "socket_server.h"
#include "net_logic.h"
#include "configure.h"

#include <pthread.h>

int main()
{
	pthread_t pthread_id[8];

	double_que* que_pool = communication_que_creat();

	//网络IO线程
	net_io_start* netio_start = net_io_start_creat(que_pool,0,"127.0.0.1",8000);
	pthread_create(&pthread_id[0],NULL,network_io_service_loop,netio_start);  

	//网络路由线程
	net_logic_start* netlog_start = net_logic_start_creat(que_pool);
	pthread_create(&pthread_id[1],NULL,net_logic_service_loop,netlog_start); 

	pthread_join(pthread_id[0],NULL);		//等待线程结束
	pthread_join(pthread_id[1],NULL);
	return 0;
}























