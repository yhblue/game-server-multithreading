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

#define MAXTHREAD     				6
#define NET_LOGIC_MAX_EVENT  		10
#define GAME_THREADING_NUM      	4

#define EVENT_QUE_NULL  	       	1
#define EVENT_THREAD_CONNECT_ERR  	2
#define EVENT_THREAD_DISCONNECT     3

#define THREAD_TYPE_NET_IO       	1
#define THREAD_TYPE_GAME_LOGIC   	2
#define THREAD_TYPE_LOG          	3
#define THREAD_TYPE_CHAT         	4

#define TO_OTHER_THREAD_QUE     	0
#define FROM_OTHER_THREAD_QUE    	1

//#pragma pack(1)
typedef struct _pack_head
{
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}pack_head;

typedef struct _threading
{
	int thread_id;
	int sock_fd;
	int type;
}threading;

typedef struct _send_game_pack
{
	q_node qnode;
}data2_game;

typedef struct _imform2_game
{
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}imform2_game;

typedef struct _double_que
{
	queue* que_to;
	queue* que_from;
}double_que;

typedef struct _net_logic
{
	double_que* que_pool;  					//存储所有线程的queue,不需分配内存
	int epoll_fd;				
	struct event* event_pool;
	bool check_que;
	int event_index;
	int event_n;	
	threading current_que;
	threading* threading_pool; 			    //线程的信息的存储结构
	int thread_socket_fd;
	unsigned char sock_id_2_threadsock_fd;  //下标是socket id，值是thread_socket_fd[] 中的下标	
	int thread_id;
}net_logic;

typedef struct _deserialize
{
	char proto_type; //存放protobuf用来序列化数据的类型
	char* buffer;    //存放反序列化之后的数据
}deserialize;

typedef struct _net_logic_start
{
	double_que* que_pool;
	int thread_id;	
}net_logic_start;

double_que* communication_que_creat()
{
	double_que* que_pool = (dou_que*)malloc(sizeof(double_que));
	for(int i=0; i<MAXTHREAD-1; i++)
	{
		que_pool[i].que_to = queue_creat();
		que_pool[i].que_from = queue_creat();
	}
	return que_pool;
}


static void unpack_user_data(char* data_pack,int len)
{
	char proto_type = data_pack[0]; 	//记录用的.proto文件中哪个message来序列化  
	char* seria_data = data_pack + 1;   //data_pack 是网络IO线程中分配的内存，反序列化完之后free掉
	deserialize* data = (deserialize*)malloc(sizeof(deserialize));

	switch(proto_type)
	{
		case LOG_REQ:
			LoginReq* msg = login_req__unpack(NULL,len,seria_data);//函数里面malloc了内存，返回给msg使用
			break;												   //当调用socket发送给游戏逻辑进程之后记得free
																   //如果msg的成员有char*类型的，记得先free这个再free msg
		case HERO_MSG_REQ:
			HeroMsg* msg = hero_msg__unpack(NULL,len,seria_data);
			break;

		case CONNECT_REQ:
			ConnectReq* msg = connect_req__unpack(NULL,len,seria_data);
			break;

		case HEART_REQ:
			HeartBeat* msg = heart_beat__unpack(NULL,len,seria_data);
			break;
	}	
	data->proto_type = proto_type;
	data->buffer = msg;
	return data;
}



static int apply_threading_id()
{
	static int id = -1;
	id ++;
	return id;	
}

static process* apply_process(net_logic* nt,int socket_fd,int p_id,bool add_epoll)
{
	threading* thread = &nt->threading_pool[p_id % GAME_THREADING_NUM];
	if(thread == NULL)
	{
		return NULL;
	}
	if(add_epoll)
	{
		if(epoll_add(nt->epoll_fd,socket_fd,thread) == -1)
		{
			return NULL;
		}
	}	
	thread->p_id = p_id;
	thread->sock_fd = socket_fd;
}

static net_logic* net_logic_creat(net_logic_start* start)
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

	size = sizeof(threading) * MAXTHREAD;
	nt->threading_pool = (threading*)malloc(size);
	if (nt->threading_pool == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:threading_pool malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		return NULL;		
	}
	for(int i=0; i<MAXTHREAD; i++)
	{
		threading* td = &nt->threading_pool[i];
		td->thread_id = 0;
		td->sock_fd = 0;
		td->type = 0;
	}

	size = sizeof(unsigned char) * MAX_SOCKET;
	nt->sock_id_2_threadsock_fd = (unsigned char*)malloc(size);
	if(nt->sock_id_2_threadsock_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:sock_id_2_threadsock_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->threading_pool);
		return NULL;			
	}
	memset(nt->sock_id_2_threadsock_fd,0,size);

	size = sizeof(int) * MAXTHREAD;
	nt->thread_socket_fd = (int*)malloc(size);
	if(nt->thread_socket_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:game_socket_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->threading_pool);
		free(nt->sock_id_2_threadsock_fd);
		return NULL;			
	}
	memset(nt->sock_id_2_threadsock_fd,0,size);

	nt->check_que = false;
	nt->nt->event_index = 0;
	nt->event_n = 0;
	nt->data_head_size = sizeof(data2_game);
	nt->inform_size = sizeof(imform2_game);
	return nt;

	nt->thread_id = start->thread_id;
	nt->que_pool = start->que_pool;
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
		threading* td = eve->s_p; 			                	 //which process socket

		switch(td->type)
		{
			case PROCESS_TYPE_NET_IO:  	  //为了唤醒epoll，去处理消息队列的信息
			case PROCESS_TYPE_GAME_LOGIC: //游戏逻辑处理进程，去处理广播信息		
				nt->check_que = true;
				nl->current_que.thread_id = td->thread_id;
				nl->current_que.type = td->type;

				if(eve->read)
				{
					int type = dispose_threading_read_msg(nt,td);
					return type;
				}
				if(eve->write)
				{

				}
				if(eve->error)
				{

				}
				break;
		}
	}
}


int dispose_threading_read_msg(net_logic* nt,threading* td)
{
	char buf[64];
	int n = read(td->sock_fd,buf,sizeof(buf));  //写到了这里接着写下去

	if(n < 0)
	{
		switch(errno)
		{ 
			case EINTR:
				fprintf(ERR_FILE,"dispose_netio_process_read_msg: socket read,EINTR\n");
				break;    	// wait for next time
			case EAGAIN:		
				fprintf(ERR_FILE,"dispose_netio_process_read_msg: socket read,EAGAIN\n");
				break;
			default:
				close(td->sock_fd);
				return EVENT_TYPE_THREAD_CONNECT_ERR;			
		}
	}
	if(n == 0) //client close,important
	{
		close(td->sock_fd);
		return EVENT_THREAD_DISCONNECT;
	}	
}


void* net_logic_event_thread(void* arg)
{
	net_logic_start* start = arg;

	net_logic *nt = net_logic_creat(start);
	if(nt == NULL)
		return -1;	
	int type = 0;

	for( ; ; )
	{
		type = net_logic_event(nt);

		switch(type)
		{
			case EVENT_TYPE_QUE_NULL:
				break;

			case EVENT_THREAD_CONNECT_ERR:
				break;

			case EVENT_THREAD_DISCONNECT:
				break;
		}
	}

}


static int dispose_queue_event(net_logic* nl)
{
	//queue* que = nl->que_pool[nl->thread_id]; //根据线程id得到相应的queue
	int thread_id = nt->current_que.thread_id;
	int thread_type nt->current_que.que_type;

	queue* que = nt->que_pool[td->thread_id];

	q_node* qnode = queue_pop(que);
	if(qnode == NULL) //队列无数据
	{
		return -1;
		//nl->thread_id = -1; //这个线程的消息队列没有数据了
	}
	else
	{
		switch(thread_type)
		{
			case THREAD_TYPE_NET_IO:
				dispose_netio_thread_que(que,qnode);
				break;

			case THREAD_TYPE_GAME_LOGIC:
				break;

			case THREAD_TYPE_LOG:
				break;

			case THREAD_TYPE_CHAT:
				break;	
		}
	}
	return 0;
}

static q_node* pack_user_data(deserialize* desseria_data,int uid_pack)
{
	q_node* qnode = (q_node*)malloc(sizeof(q_node));
	if(pack == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:qnode malloc failed\n");
		return NULL;
	}

	qnode->msg_type = TYPE_DATA;
	qnode->uid = uid_pack;
	qnode->len = 0;
	qnode->next = NULL;

	qnode->proto_type = desseria_data->proto_type;
	qnode->buffer = desseria_data->buffer; 

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

static void dispose_netio_thread_que(queue* que,q_node qnode)
{
	char type = qnode->type;  		// 'D' 'S' 'C'
	int uid = qnode->uid;
	char* data = qnode->buffer;
	int content_len = qnode->len; //内容的长度(即客户端原始发送上来的数据的长度)	

	switch(type)
	{
		case TYPE_DATA:     // 'D' 数据包--pack--send
			deserialize* deseria_data = unpack_user_data(data,content_len);    //upack
			data2_game* send_data = pack_user_data(data,uid);        		  //pack
			send_data_2_game_logic();							  //send
			break;

		case TYPE_CLOSE:    // 'C'
		case TYPE_SUCCESS:  // 'S' 
			imform2_game* send_inform = pack_inform_data(content_len,uid,type);  //len暂时不需要用到
			send_inform_2_game_logic(nl,send_inform);
			break;
	}		
} 


static int send_data_2_game_logic(net_logic* nl,data2_game* send_data)
{
	int uid = send_data->qnode.uid;           
	int que_id = route_get_queue(nl);         //根据路由表得到线程的id

	queue* que = route_get_queue(nl);  	  	 //得到应该把数据分发给哪一个队列
	q_node* qnode = send_data->qnode;     	
	queue_push(que,qnode);	                 //发送数据给其他线程相当于向对应的消息队列中push数据
}

//路由表的功能是:根据 uid和 发送/接收 队列得到相应的队列
int route_get_queue(net_logic* nl,int que_type)
{
	int index = //继续写这里
	double_que* dou_que = nil->que_pool[]
	if(que_type == TO_OTHER_THREAD_QUE)  //current thread to other thread
	{
		que = dou_que[TO_OTHER_THREAD_QUE];
	}
	else if(que_type == FROM_OTHER_THREAD_QUE)
	{
		que = dou_que[FROM_OTHER_THREAD_QUE];
	}
	return que;
}