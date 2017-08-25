#include "net_logic.h"
#include "message.pb-c.h"
#include "lock_queue.h"
#include "err.h"
#include "socket_epoll.h"
#include "message.pb-c.h"
#include "configure.h"
#include "proto.h"
#include "socket_server.h"
#include "port.h"

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

#define MAX_SERVICE      			8
#define NET_LOGIC_MAX_EVENT  	   10
#define GAME_THREADING_NUM      	4

#define EVENT_TYPE_QUE_NULL  	    1
#define EVENT_THREAD_CONNECT_ERR  	2
#define EVENT_THREAD_DISCONNECT     3

#define SERVICE_TYPE_NET_IO       	1
#define SERVICE_TYPE_GAME_LOGIC   	2
#define SERVICE_TYPE_LOG          	3
#define SERVICE_TYPE_CHAT         	4

#define SERVICE_ID_NETWORK_IO       0
#define SERVICE_ID_NET_ROUTE        1
#define SERVICE_ID_LOG              2
#define SERVICE_ID_DB               3
#define SERVICE_ID_GAME_FIRST       4
#define SERVICE_ID_GAME_SECOND      5
#define SERVICE_ID_GAME_THIRD       6
#define SERVICE_ID_GAME_FOURTH      7

#define GAME_LOGIC_SERVICE_NUM      4
#define GAME_LOGIC_SERVER_FIRST     0
#define GAME_LOGIC_SERVER_SECOND    1
#define GAME_LOGIC_SERVER_THIRD     2
#define GAME_LOGIC_SERVER_FOURTH    3



typedef struct _pack_head
{
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}pack_head;

typedef struct _service
{
	int thread_id;
	int sock_fd;
	int type;
}service;

typedef struct _send_game_pack
{
	q_node qnode;
}data2_game;

typedef struct _imform2_game
{
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}imform2_game;

typedef struct _router
{
	unsigned char sock_id2game_logic[MAX_SOCKET];			//用户socket id 到 游戏逻辑服务的id 0-3
	int online_player[GAME_LOGIC_SERVICE_NUM];  			//记录每个游戏服务的人数 
	queue* que_2_gamelogic[GAME_LOGIC_SERVICE_NUM]; 		//net logic 服务到 game logic服务各自的消息队列
}router;

typedef struct _net_logic
{			
	queue* que_pool;						//存储所有线程的queue,不需分配内存
	queue* current_que;                     //有数据到来的que
	int epoll_fd;							
	struct event* event_pool;
	bool check_que;
	int event_index;
	int event_n;	
	service* current_serice;            	//epoll return 
	service* service_pool; 			    	//线程的信息的存储结构,用于epoll
	int thread_id;
	router route;   						//路由表
}net_logic;

typedef struct _deserialize
{
	char proto_type; 	//存放 protobuf 用来序列化数据的类型
	char* buffer;    	//存放反序列化之后的数据
}deserialize;


queue* message_que_creat()
{
	queue* que_pool = (queue*)malloc(sizeof(queue)*MESSAGE_QUEUE_NUM);
	if(que_pool == NULL)
	{
		fprintf(ERR_FILE,"message_que_creat:que_pool malloc failed\n");
		return NULL;		
	}
	return que_pool;
}

uint8_t route_get_gamelogic_id(net_logic* nl)
{
	int gamelogic01_player = nt->route.online_player[GAME_LOGIC_SERVER_FIRST];
	int gamelogic02_player = nt->route.online_player[GAME_LOGIC_SERVER_SECOND];
	int gamelogic03_player = nt->route.online_player[GAME_LOGIC_SERVER_THIRD];
	int gamelogic04_player = nt->route.online_player[GAME_LOGIC_SERVER_FOURTH];

	uint8_t id1 = (gamelogic01_player >= gamelogic02_player)? GAME_LOGIC_SERVER_FIRST:GAME_LOGIC_SERVER_SECOND;
	uint8_t id2 = (gamelogic03_player >= gamelogic04_player)? GAME_LOGIC_SERVER_THIRD:GAME_LOGIC_SERVER_FOURTH;

	uint8_t ret = (nt->route.online_player[id1] >= nt->route.online_player[id2])? id1 : id2;

	return ret;
}

void route_set_sock_id2game_logic(net_logic* nl,int socket_id,uint8_t game_id)
{
	nl->route.sock_id2game_logic[socket_id] = game_id;
}

//这个函数看看能不能改一下，这个函数必须依赖网络IO线程按照指定格式读，这样效率低
static deserialize* unpack_user_data(unsigned char * data_pack,int len)
{
	unsigned char proto_type = data_pack[0]; 	 //记录用的.proto文件中哪个message来序列化  
	unsigned char* seria_data = data_pack + 1;   //data_pack 是网络IO线程中分配的内存，反序列化完之后free掉
	deserialize* data = (deserialize*)malloc(sizeof(deserialize));
	int data_len = len -1; //减去第一个字节包头
	void * msg = NULL;
	switch(proto_type)
	{
		case LOG_REQ:
			msg = login_req__unpack(NULL,data_len,seria_data);//函数里面malloc了内存，返回给msg使用
			break;		

		case HERO_MSG_REQ:
			msg = hero_msg__unpack(NULL,data_len,seria_data);
			break;			

		case CONNECT_REQ:
			msg = connect_req__unpack(NULL,data_len,seria_data);
			break;				

		case HEART_REQ:
			msg = heart_beat__unpack(NULL,data_len,seria_data);
			break;			
	}	
	data->proto_type = proto_type;
	data->buffer = msg;
	free(data_pack); 	//客户端发送上来的数据反序列化之后释放掉内存
	return data;
}


static net_logic* net_logic_creat(net_logic_start* start)
{
	net_logic *nt = (net_logic*)malloc(sizeof(net_logic));
	if(nt == NULL)
		return NULL;
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

	size = sizeof(service) * MAX_SERVICE;
	nt->service_pool = (service*)malloc(size);
	if (nt->service_pool == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:service_pool malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		return NULL;		
	}
	for(int i=0; i<MAX_SERVICE; i++)
	{
		service* sv = &nt->service_pool[i];
		sv->thread_id = 0;
		sv->sock_fd = 0;
		sv->type = 0;
	}

	nt->check_que = false;
	nt->event_index = 0;
	nt->event_n = 0;

	nt->thread_id = start->thread_id;
	nt->que_pool = start->que_pool;

	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_FIRST]  = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_FIRST];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_SECOND] = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_SECOND];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_THIRD]  = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_THIRD];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_FOURTH] = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_FOURTH];

	for(int i=0; i<MAX_SOCKET; i++)
	{
		nt->sock_id2game_logic[i] = 0;
	}
	for(int i=0; i<GAME_LOGIC_SERVICE_NUM; i++)
	{
		online_player[i] = 0;
	}
	return nt;
}

static q_node* pack_user_data(deserialize* desseria_data,int uid_pack)
{
	q_node* qnode = (q_node*)malloc(sizeof(q_node));
	if(qnode == NULL)
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

	return qnode;
}

//测试 client -> netio -> net_logic打印反序列化之后的数据 
void send_data_test(queue* que,q_node* qnode)
{	
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"send_data_test:qnode is null\n");
		return NULL;		
	}
	printf("send_data_test command\n");
	char msg_type = qnode->msg_type;
	printf("msg_type = %c",msg_type);
	char proto_type = qnode->proto_type;
	printf("proto_type = %c",proto_type);
	unsigned char* buffer = qnode->buffer;

	switch(proto_type)
	{
		case LOG_REQ:

			break;		

		case HERO_MSG_REQ:
			{
				HeroMsg* msg = (HeroMsg*)buffer;
				printf("type is HeroMsg,msg->uid = %d,msg->point_x = %d,msg->point_y = %d\n",msg->uid,msg->point_x,msg->point_y);
				break;	
			}
			
		case CONNECT_REQ:

			break;				

		case HEART_REQ:

			break;			
	}
}


static int send_data_2_game_logic(net_logic* nl,q_node* qnode)
{
	// int uid = send_data->qnode.uid;           
	// int que_id = route_get_queue(nl);         //根据路由表得到线程的id

	// queue* que = route_get_queue(nl);  	  	 //得到应该把数据分发给哪一个队列
	// q_node* qnode = send_data->qnode;     	
	// queue_push(que,qnode);	                 //发送数据给其他线程相当于向对应的消息队列中push数据
	return 0;
}

//路由表的功能是:根据 uid和 发送/接收 队列得到相应的队列
static int route_get_queue(net_logic* nl,int que_type)
{
	// int index = //继续写这里
	// double_que* dou_que = nil->que_pool[]
	// if(que_type == TO_OTHER_THREAD_QUE)  //current thread to other thread
	// {
	// 	que = dou_que[TO_OTHER_THREAD_QUE];
	// }
	// else if(que_type == FROM_OTHER_THREAD_QUE)
	// {
	// 	que = dou_que[FROM_OTHER_THREAD_QUE];
	// }
	// return que;
	return 0;
}

static imform2_game* pack_inform_data(unsigned char len_pack,int uid_pack,char type_pack)
{
	// imform2_game* pack = (imform2_game*)malloc(sizeof(imform2_game));
	// if(pack == NULL)
	// {
	// 	fprintf(ERR_FILE,"pack_inform:pack malloc failed\n");
	// 	return NULL;
	// }	
	// pack->content_len = len_pack;
	// pack->msg_type = type_pack;
	// pack->uid = uid_pack;

	// return pack;
	return NULL;
}

static void dispose_netio_thread_que(queue* que,q_node* qnode)
{
	char type = qnode->msg_type;  		// 'D' 'S' 'C'
	int uid = qnode->uid;
	unsigned char* data = qnode->buffer;
	int content_len = qnode->len; 		//内容的长度(即客户端原始发送上来的数据的长度)	

	switch(type)
	{
		case TYPE_DATA:     			// 'D' data--upack--pack--send
			{
				deserialize* deseria_data = unpack_user_data(data,content_len);    	//upack
				q_node* send_qnode = pack_user_data(deseria_data,uid);        		//pack
				//send_data_2_game_logic(send_qnode);							  	//send
				printf("test start:\n");
				send_data_test(que,send_qnode);
				break;				
			}

		case TYPE_CLOSE:    			// 'C'
		case TYPE_SUCCESS:  			// 'S' 
			{
				imform2_game* send_inform = pack_inform_data(content_len,uid,type);  //len暂时不需要用到
				//send_inform_2_game_logic(nl,send_inform);
				break;				
			}

	}		
} 

static int dispose_queue_event(net_logic* nt)
{
	int service_type = nt->current_serice->type;
	queue* que = nt->current_que;
	q_node* qnode = queue_pop(que);
	if(qnode == NULL) //队列无数据
	{
		return -1;
	}
	else
	{
		switch(service_type)
		{
			case SERVICE_TYPE_NET_IO:			
				dispose_netio_thread_que(que,qnode);
				break;

			case SERVICE_TYPE_GAME_LOGIC:
				break;

			case SERVICE_TYPE_LOG:
				break;

			case SERVICE_TYPE_CHAT:
				break;	
		}
	}
	return 0;
}

static int dispose_threading_read_msg(net_logic* nt,service* sv)
{
	char buf[64];
	int n = read(sv->sock_fd,buf,sizeof(buf));  //写到了这里接着写下去
	buf[n] = '\0';
	//printf("%s\n",buf);
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
				close(sv->sock_fd);
				return EVENT_THREAD_CONNECT_ERR;			
		}
	}
	if(n == 0) //client close,important
	{
		close(sv->sock_fd);
		return EVENT_THREAD_DISCONNECT;
	}	
}

static int net_logic_event(net_logic *nt)
{
	for( ; ; )
	{
		if(nt->check_que == true) 
		{
			printf("handle que event start!\n");
			int ret = dispose_queue_event(nt);
			printf("handle que event! end\n");
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
				nt->event_n = 0;
				return -1;
			}	
			nt->event_index = 0;
		}
		nt->event_index = 0;			
		struct event* eve = &nt->event_pool[nt->event_index++];  //read or write
		service* sv = eve->s_p; 			                	 //which process socket

		switch(sv->type)			
		{
			case SERVICE_TYPE_NET_IO:  	  	//为了唤醒epoll，去处理消息队列的信息
				nt->current_que = &nt->que_pool[QUE_ID_NETIO_2_NETLOGIC];	
				nt->current_serice = sv;	
				nt->check_que = true;
				if(eve->read)
				{
					int type = dispose_threading_read_msg(nt,sv);
					printf("netlogic epoll work\n");
					return type;
				}
				break;

			case SERVICE_TYPE_GAME_LOGIC: 	//游戏逻辑处理进程，去处理广播信息		
				nt->current_que = &nt->que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
				nt->check_que = true;
				nt->current_serice = sv;

				if(eve->read)
				{
					int type = dispose_threading_read_msg(nt,sv);
					printf("netlogic epoll work\n");
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


static int connect_netio_service(net_logic* nl,char* netio_addr,int netio_port)
{
    struct sockaddr_in netio_server_addr;  //存放服务地址信息
    struct sockaddr_in netlog_service_addr;

    //creat socket
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if( sockfd == -1 )
    {
       fprintf(ERR_FILE,"connect_netio_service:socket creat failed\n");
       return -1;
    }

    netlog_service_addr.sin_family = AF_INET;
    netlog_service_addr.sin_port = htons(NETLOGIC_SERVICE_PORT);		//8001
    bind(sockfd,(struct sockaddr*)(&netlog_service_addr),sizeof(netlog_service_addr));

    //set service address
    memset(&netio_server_addr,0,sizeof(netio_server_addr));
    netio_server_addr.sin_family = AF_INET;
    netio_server_addr.sin_port = htons(netio_port);
    netio_server_addr.sin_addr.s_addr = inet_addr(netio_addr);

    for( ;; )
    {
	    if((connect(sockfd,(struct sockaddr*)(&netio_server_addr),sizeof(struct sockaddr))) == -1)
	    {
	        fprintf(ERR_FILE,"connect_netio_service:netlogic_thread disconnect\n");
	        //return -1;
	        sleep(5);
	    }
	    else
	    {
		    //connect sussess
			service* sv = &nl->service_pool[SERVICE_ID_NETWORK_IO];
		    sv->thread_id = SERVICE_ID_NETWORK_IO;
		    sv->sock_fd = sockfd;
		    sv->type = SERVICE_TYPE_NET_IO;
		    
		    //add to epoll
		    printf("net_logic service connect to net_io service success!\n");
		    if(epoll_add(nl->epoll_fd,sockfd,sv) == -1)
		    {
		       fprintf(ERR_FILE,"connect_netio_service:epoll_add failed\n");
		       return -1;    	
		    } 	
		    return 0;    	
	    }	
  	
    }
    return 0;
}



static int service_connect_establish(net_logic_start* start,net_logic* nl)
{
	char* netio_addr = start->netio_addr;
	int netio_port = start->netio_port;
	if(connect_netio_service(nl,netio_addr,netio_port) == -1)
	{
       fprintf(ERR_FILE,"service_connect_establish:connect_netio_service failed\n");
       return -1;			
	}
	return 0;
}

//configure* config,
net_logic_start* net_logic_start_creat(queue* que_pool)
{
	net_logic_start* start = malloc(sizeof(net_logic_start));
	if(start == NULL)
	{
       fprintf(ERR_FILE,"net_logic_start_creat:start malloc failed\n");
       return NULL;			
	}
	start->que_pool = que_pool;
	start->thread_id = 1;
	start->netio_addr = "127.0.0.1";
	start->netio_port = 8000;

	return start;
}

void* net_logic_service_loop(void* arg)
{
	net_logic_start* start = arg;
	net_logic *nt = net_logic_creat(start);
	if(nt == NULL)
	{
       fprintf(ERR_FILE,"net_logic_service_loop:connect_netio_service disconnect\n");
       return NULL;		
	}
	if(service_connect_establish(start,nt) == -1)
	{
       fprintf(ERR_FILE,"net_logic_service_loop:service_connect_establish failed\n");
       return NULL;			
	}
	int type = 0;

	for( ; ; )
	{
		type = net_logic_event(nt);
		switch(type)
		{
			case EVENT_TYPE_QUE_NULL:
				printf("queue is null\n");
				break;

			case EVENT_THREAD_CONNECT_ERR:
				break;

			case EVENT_THREAD_DISCONNECT:
				break;
		}
		// printf("net_logic_service_loop running!\n");
		// sleep(10);
	}
	return NULL;
}












/*
static int apply_threading_id()
{
	static int id = -1;
	id ++;
	return id;	
}

static process* apply_process(net_logic* nt,int socket_fd,int p_id,bool add_epoll)
{
	service* sv = &nt->service_pool[p_id % GAME_THREADING_NUM];
	if(sv == NULL)
	{
		return NULL;
	}
	if(add_epoll)
	{
		if(epoll_add(nt->epoll_fd,socket_fd,sv) == -1)
		{
			return NULL;
		}
	}	
	sv->p_id = p_id;
	sv->sock_fd = socket_fd;
}


	size = sizeof(unsigned char) * MAX_SOCKET;
	nt->sock_id_2_threadsock_fd = (unsigned char*)malloc(size);
	if(nt->sock_id_2_threadsock_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:sock_id_2_threadsock_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->service_pool);
		return NULL;			
	}
	memset(nt->sock_id_2_threadsock_fd,0,size);

	size = sizeof(int) * MAX_SERVICE;
	nt->thread_socket_fd = (int*)malloc(size);
	if(nt->thread_socket_fd == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:game_socket_fd malloc failed\n");
		epoll_release(efd);
		free(nt->event_pool);
		free(nt->service_pool);
		free(nt->sock_id_2_threadsock_fd);
		return NULL;			
	}
	memset(nt->sock_id_2_threadsock_fd,0,size);


 */