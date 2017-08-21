#ifndef  _SOCKET_SERVER_H
#define _SOCKET_SERVER_H

//返回的result的类型
#define SOCKET_DATA 0			// 有数据到来
#define SOCKET_CLOSE 1			// 连接关闭
#define SOCKET_SUCCESS 2		// 连接成功建立（主动或者被动，并且已加入到epoll）
#define SOCKET_ACCEPT 3			// 被动连接建立（即accept成功返回已连接套接字）但未加入到epoll
#define SOCKET_ERROR 4			// 发生错误
#define SOCKET_EXIT 5			// 退出事件
#define SOCKET_IGNORE 6         // 忽略,无需处理



struct socket_message {
	int id;
	int lid_size;	           // for accept,ud is id，for data,size of data
	char* data;    				//指向返回的数据
};

typedef struct _net_io_start
{
	double_que* que_pool;
	int thread_id;
	int port;
	char* address;
}net_io_start;

struct socket_server* socket_server_create();
int socket_server_event(struct socket_server *ss, struct socket_message * result);
int socket_server_listen(struct socket_server *ss,const char* host,int port,int backlog);
int socket_server_start(struct socket_server *ss,int id);
void socket_server_release(struct socket_server *ss);
void read_test(struct socket_server* ss,int id,const char* data,int size,struct socket_message *result);
void* socket_server_thread_loop(void* arg);
net_io_start* net_io_start_creat(double_que* que_pool,int thread_id,char*address,int port);

#endif

