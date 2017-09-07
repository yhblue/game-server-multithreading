#ifndef  _SOCKET_SERVER_H
#define _SOCKET_SERVER_H

#include "net_logic.h"
#include "configure.h"

//返回的result的类型
#define SOCKET_DATA 0			// 有数据到来
#define SOCKET_CLOSE 1			// 连接关闭
#define SOCKET_SUCCESS 2		// 连接成功建立（主动或者被动，并且已加入到epoll）
#define SOCKET_ACCEPT 3			// 被动连接建立（即accept成功返回已连接套接字）但未加入到epoll
#define SOCKET_ERROR 4			// 发生错误
#define SOCKET_EXIT 5			// 退出事件
#define SOCKET_IGNORE 6         // 忽略,无需处理
#define SOCKET_NETLOGIC_QUE  7

#define MAX_SOCKET 					32*1024           //最多支持32k个socket连接 

struct socket_message {
	int id;
	int lid_size;	           // for accept,ud is id for data,size of data
	char* data;    				//指向返回的数据
};

typedef struct _net_io_start
{
	queue* que_pool;
	int service_id;

	int port;
	char* address;
}net_io_start;


void socket_keepalive(int fd);
int send_msg2_service(int socket);
net_io_start* net_io_start_creat(queue* que_pool,configure* conf,int service_id);
void* network_io_service_loop(void* arg);

#endif

