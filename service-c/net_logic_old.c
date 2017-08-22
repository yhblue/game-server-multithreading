//实现功能:
//1)解析 protobuf 协议,转发到相应的游戏逻辑进程
//2)对游戏逻辑进程要发送的数据使用 protobuf 系列化，通知网络发送线程发送
#if 0
#include "net_logic.h"
#include "message.pb-c.h"
#include "lock_queue.h"
#include "err.h"
#include "socket_epoll.h"
#include "message.pb-c.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define NET_LOGIC_MAX_EVENT  	 10
#define GAME_PROCESS_NUM      	  4

#define EVENT_TYPE_QUE_NULL  	  1

#define PROCESS_TYPE_NET_IO       1
#define PROCESS_TYPE_GAME_LOGIC   2


#pragma pack(1)
typedef struct _pack_head
{
	//unsigned char content_len;
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}pack_head;


typedef struct _send_game_pack
{
	pack_head head;
	char* content;
}data2_game;

typedef pack_head imform2_game; 

// typedef struct _imform_2_game  //通知
// {
// 	unsigned char len;
// 	char msg_type;     //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
// 	int uid;
// }imform2_game;


#define MAXTHREAD     6
#pragma pack()

typedef struct _process
{
	int p_id;
	int sock_fd;
	int type;
}process;

typedef struct _net_logic
{
	queue* que;
	int epoll_fd;
	int game_socket_fd[];
	unsigned char sock_id_2_game_fd; //下标是socket id，值是game_socket_fd[] 中的下标
	int online_player[]; 			//下标是一个地图编号，对应的值是这一块地图上的玩家人数
	struct event* event_pool;
	process* game_process_pool; 
	bool check_que;
	int event_index;
	int event_n;
	int pack_head_size;
	queue* que_pool[MAXTHREAD];  //存储所有线程的queue
}net_logic;

static data2_game* pack_user_data(char* seria_data,unsigned char len_pack,int uid_pack,char type_pack)
{
	data2_game* pack = (data2_game*)malloc(sizeof(data2_game));
	if(pack == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:pack malloc failed\n");
		return NULL;
	}

	pack->head.content_len = len_pack; //存储序列化数据的buf的长度
	pack->head.msg_type = type_pack;
	pack->head.uid = uid_pack;
	pack->content = seria_data; 

	return pack;
}

static imform2_game* pack_inform_data(unsigned char len_pack,int uid_pack,char type_pack)
{
	imform2_game* pack = (imform2_game*)malloc(sizeof(imform2_game));
	if(pack == NULL)
	{
		fprintf(ERR_FILE,"pack_inform:pack malloc failed\n");
		return NULL;
	}	
	pack->content_len = len_pack;
	pack->msg_type = type_pack;
	pack->uid = uid_pack;

	return pack;
}


static int apply_process_id()
{
	static int id = -1;
	id ++;
	return id;	
}

static process* apply_process(net_logic* nt,int socket_fd,int p_id,bool add_epoll)
{
	process* g_p = &nt->game_process_pool[p_id % GAME_PROCESS_NUM];
	if(g_p == NULL)
	{
		return NULL;
	}
	if(add_epoll)
	{
		if(epoll_add(nt->epoll_fd,socket_fd,g_p) == -1)
		{
			return NULL;
		}
	}	
	g_p->p_id = p_id;
	g_p->sock_fd = socket_fd;
}

static void close_proces_fd()
{

}

static int connect_game_logic()
{

}
//----------------------------------------------------------------------------------------------------------------------
static int dispose_queue_event(net_logic* nl)
{
	queue* que = nl->que;
	q_node* qnode = queue_pop(que);
	if(qnode == NULL) //队列无数据
	{
		return -1;
		nl->thread_id = -1; //这个线程的消息队列没有数据了
	}
	else
	{
		char type = qnode->type;  		// 'D' 'S' 'C'
		int uid = qnode->uid;
		char* data = qnode->buffer;
		int content_len = qnode->len; //内容的长度(即客户端原始发送上来的数据的长度)

		switch(type)
		{
			case TYPE_DATA:     // 'D'  数据包--pack--send
				send_data = pack_user_data(data,len,uid,type); 		  //pack
				send_data_2_game_logic(nl,send_data,len,uid); //send
				break;

			case TYPE_CLOSE:    // 'C'
			case TYPE_SUCCESS:  // 'S' 
				imform2_game* send_inform = pack_inform_data(content_len,uid,type);  //len暂时不需要用到
				send_inform_2_game_logic(nl,send_inform);
			break;
		}		
	}
	return 0;
}



//发送完成之后记得释放内存	//错误
//别释放内存
static int send_data_2_game_logic(net_logic* nl,data2_game* pack_data)
{
	int uid = pack_data->head.uid;
	int p_socketfd = nl->game_socket_fd[nt->sock_id_2_game_fd[uid]];
	int head_size = nt->data_head_size;
	int content_len = pack_data->head.content_len;

	int n = write(p_socketfd,pack_data->head,head_size);  //先发送数据包头部
	if(n == -1)
		goto _err;
	assert(n == head_size);

	n = write(p_socketfd,pack_data->content,content_len);
	if(n == -1)
		goto _err;
	assert(n == content_len);

	if(pack_data->content != NULL)
	{
		free(pack_data->content);
		pack_data->content = NULL;
	}
	if(pack_data != NULL)
	{
		free(pack_data);
	}

_err:	
	if(n == -1)
	{
		switch(errno)
		{
			case EINTR:
			case EAGAIN:
				n = 0;
				break;
			default:
				fprintf(stderr, "send_data_2_game_logic: write to process fd=%d error.",p_socketfd);			
				return -1;
		}
	}	
}



static int send_inform_2_game_logic(net_logic* nl,imform2_game* inform)
{
	int p_socketfd = nl->game_socket_fd[nt->sock_id_2_game_fd];
	int head_size = nt->inform_size;

	int n = write(p_socketfd,inform,head_size);  //发送通知只需要发送一次消息头部即可，不需要发送数据
	if(n == -1)
	{
		switch(errno)
		{
			case EINTR:
			case EAGAIN:
				n = 0;
				break;
			default:
				fprintf(stderr, "send_data_2_game_logic: write to process fd=%d error.",p_socketfd);			
				return -1;
		}
	}	
	assert(n == head_size);
	if(inform != NULL)
	{
		free(inform);
		inform = NULL;
	}
}

static net_logic* net_logic_creat()
{
	net_logic *nt = (net_logic*)malloc(sizeof(struct net_logic));
	if(nt == NULL)
		return -1;
	int efd = epoll_init();
	if(efd_err(efd))
	{
		fprintf(ERR_FILE,"net_logic_creat:epoll create failed\n");
		return NULL;
	}
	nt->epoll_fd = efd;

	int size = sizeof(struct event) * NET_LOGIC_MAX_EVENT;
	nt->event_pool = (struct event*)malloc(size);
	if(nt->event_pool == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:event_pool malloc failed\n");
		epoll_release(efd);
		return NULL;	
	}
	memset(nt->event_pool,0,size);

	size = sizeof(process) * GAME_PROCESS_NUM;
	nt->game_process_pool = (process*)malloc(size);
	if (nt->game_process_pool == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:event_pool malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		return NULL;		
	}
	for(int i=0; i<GAME_PROCESS_NUM; i++)
	{
		process* gp = &nt->game_process_pool[i % GAME_PROCESS_NUM];
		gp->p_id = 0;
		gp->sock_fd = 0;
		gp->type = 0;
	}

	size = sizeof(unsigned char) * MAX_SOCKET;
	nt->sock_id_2_game_fd = (unsigned char*)malloc(size);
	if(nt->sock_id_2_game_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:sock_id_2_game_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->game_process_pool);
		return NULL;			
	}
	memset(nt->sock_id_2_game_fd,0,size);

	size = sizeof(int) * GAME_PROCESS_NUM;
	nt->game_socket_fd = (int*)malloc(size);
	if(nt->game_socket_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:game_socket_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->game_process_pool);
		free(nt->game_socket_fd);
		return NULL;			
	}
	memset(nt->game_socket_fd,0,size);

	nt->check_que = false;
	nt->nt->event_index = 0;
	nt->event_n = 0;
	nt->data_head_size = sizeof(data2_game);
	nt->inform_size = sizeof(imform2_game);
	return nt;
}



void* net_logic_event_thread(void* arg)
{
	net_logic *nt = net_logic_creat();
	if(nt == NULL)
		return -1;	
	nt->que = arg;
	int type = 0;

	for( ; ; )
	{
		type = net_logic_event(nt);
	}

}


int net_logic_event(net_logic *nt)
{
	for( ; ; )
	{
		if(nt->check_que == true)
		{
			int ret = dispose_queue_event(nt);
			if(ret == -1) //queue is null
			{
				nt->check_que = false;
				return EVENT_TYPE_QUE_NULL;
			}
			else
			{
				continue;
			}
		}
		if(nt->event_index == nt->event_n)
		{
			nt->event_n = sepoll_wait(nt->epoll_fd,nt->event_pool,NET_LOGIC_MAX_EVENT);
			if(nt->event_n <= 0) //error
			{
				fprintf(ERR_FILE,"net_logic_event:sepoll_wait return error event_n\n");
				ss->event_n = 0;
				return -1;
			}	
			nt->event_index = 0;
		}
		ss->event_index = 0;			
		struct event* eve = &nt->event_pool[ss->event_index++];  //read or write
		process* pc = eve->s_p; 			                	 //which process socket

		switch(g_p->type)
		{
			case PROCESS_TYPE_NET_IO:  //为了唤醒epoll，去处理消息队列的信息
				nt->check_que = true;
				if(eve->read)
				{

				}
				break;

			case PROCESS_TYPE_GAME_LOGIC:
				break;
		}

	}
}

int dispose_netio_process_read_msg(net_logic* nt,process* pc)
{
	char buf[64];
	read(pc->sock_fd,buf,sizeof(buf));  //写到了这里接着写下去

}

int dispose_game_process_read_msg(net_logic* nt)
{

}

//有新的游戏逻辑进程连接上来就调用该函数
//把游戏逻辑进程的 socket 加入到 epoll 中管理
int proces_socket_start(net_logic *nt,int sock_fd)
{
	// int p_id = apply_process_id();

	// struct socket* cs = apply_process(nt,sock_fd,p_id,true);
	// if(cs == NULL) 
	// {
	// 	close(sock_fd); //err close
	// 	fprintf(ERR_FILE,"proces_socket_start: apply_process failed\n");
	// 	return -1;
	// }
	// return 0;
}








/*
typedef struct _upack_buf
{
	unsigned char buffer_len;
	int uid;
	char msg_type;   //通知游戏逻辑进程这个消息是哪种类型
	char data_type;  //当消息类型为客户端数据时候，记录使用的数据结构类型
	char *buffer; 	//客户端发过来的经过反序列化之后的原始数据

}upack_buf;




	// send_data->total_len = XX;
	// send_data->head.conten_len = XX;
	// send_data->head.msg_type = type; 		//'D'
	// send_data->head.data_type = proto_type;
	// send_data->head.uid = uid;
	// send_data->conten = msg; //msg指向的一块内存



//写一个函数，能够根据传递过来的socket的id知道它现在在哪个游戏逻辑进程
//得到该进程的连接上来的 socket fd
int get_game_logic_id(int socket_id)
{
	
}


 */

#endif