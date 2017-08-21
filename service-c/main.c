#include "socket_server.h"

#define THREAD_ID_NET_IO 	  0
#define THREAD_ID_NET_LOGIC   1


void main()
{
	pthread_t pthread_id[6];

	double_que* que_pool = communication_que_creat();

	net_io_start* netio_start = net_io_start_creat(que_pool,THREAD_ID_NET_IO,"127.0.0.1",8888);
	pthread_create(&pthread_id[0],NULL,socket_server_thread_loop,netio_start);  


	pthread_join(pthread_id[0],NULL);//等待线程结束
}























