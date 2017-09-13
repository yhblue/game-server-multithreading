#include "net_logic.h"
#include "message.pb-c.h"
#include "lock_queue.h"
#include "socket_epoll.h"
#include "proto.h"
#include "socket_server.h"
#include "configure.h"
#include "game_logic.h"
#include "port.h"
#include "err.h"

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

#define NET_LOGIC_MAX_EVENT  	   10
#define GAME_THREADING_NUM      	4

#define EVENT_TYPE_QUE_NULL  	    1
#define EVENT_THREAD_CONNECT_ERR  	2
#define EVENT_THREAD_DISCONNECT     3

#define SERVICE_TYPE_NET_IO       	1
#define SERVICE_TYPE_NETLOGIC		2
#define SERVICE_TYPE_GAME_LOGIC   	3
#define SERVICE_TYPE_LOG          	4
#define SERVICE_TYPE_CHAT         	5


#define SERVICE_ID_NETWORK_IO       0
#define SERVICE_ID_NET_ROUTE        1
#define SERVICE_ID_LOG              2
#define SERVICE_ID_DB               3
#define SERVICE_ID_GAME_FIRST       4
#define SERVICE_ID_GAME_SECOND      5
#define SERVICE_ID_GAME_THIRD       6
#define SERVICE_ID_GAME_FOURTH      7

#define PLAYER_TYPE_INVALID		   -1

#define PROTO_HEAD_SIZE				2
#define PROTO_TYPE_SIZE 			1
#define HEAD_PROTO_TYPE_INDEX		0
#define HEAD_PROTO_SIZE_INDEX		1



typedef struct _pack_head
{
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
	int uid;     	  //socket的id(或者说是用户的id)
}pack_head;

typedef struct _service
{
	int service_id;
	int sock_fd;
	int type;
}service;


typedef struct _router
{
	char sock_id2game_logic[MAX_SOCKET];					//用户socket id 到 游戏逻辑服务的id 0-3
	int online_player[GAME_LOGIC_SERVICE_NUM];  			//记录每个游戏服务的人数 
	queue* que_2_gamelogic[GAME_LOGIC_SERVICE_NUM]; 		//net logic 服务到 game logic服务各自的消息队列
	int gamelog_socket[GAME_LOGIC_SERVICE_NUM];             //与game_logic 服务通信的socket
	int game_service_id;									//存储当前 socket 所在的游戏逻辑服务 id
}router;

typedef struct _net_logic
{			
	queue* que_pool;						//存储所有线程的queue,不需分配内存
	queue* current_que;                     //有数据到来的que
	queue* network_que;
	int epoll_fd;					
	int netio_netroute_socket;					
	struct event* event_pool;
	bool check_que;
	int event_index;
	int event_n;	
	service* current_serice;            	//epoll return 
	service* service_pool; 			    	//线程的信息的存储结构,用于epoll
	int service_id;
	router route;   						//路由表
	int listen_fd;
	char* serv_addr;
	int serv_port;
	bool socket_send;						//为 true 才会向另一个服务发送一个字节的信息，唤醒对方，然后立刻
											//修改为false,只有当对方服务进入了休眠状态，在进入之前发送一个字节给
}net_logic;									//自己，告诉它已经进入休眠，修改为true

typedef struct _deserialize
{
	char proto_type; 	//存放 protobuf 用来序列化数据的类型
	char* buffer;    	//存放反序列化之后的数据
}deserialize;

msg_head* msg_head_create(char msg_type,char proto_type,int uid,int len)
{
	msg_head* head = (msg_head*)malloc(sizeof(msg_head));
	if(head == NULL)
	{
		return NULL;
	}
    head->msg_type = msg_type;
    head->proto_type = proto_type;
    head->uid = uid;
    head->len = len;	

    return head;

}

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

//为一个新的 socket 分配到一个游戏逻辑服务的 id
uint8_t route_distribute_gamelogic(net_logic* nl)
{
	int gamelogic01_player = nl->route.online_player[GAME_LOGIC_SERVER_FIRST];
	int gamelogic02_player = nl->route.online_player[GAME_LOGIC_SERVER_SECOND];
	int gamelogic03_player = nl->route.online_player[GAME_LOGIC_SERVER_THIRD];
	int gamelogic04_player = nl->route.online_player[GAME_LOGIC_SERVER_FOURTH];

	uint8_t id1 = (gamelogic01_player < gamelogic02_player)? GAME_LOGIC_SERVER_FIRST:GAME_LOGIC_SERVER_SECOND;
	uint8_t id2 = (gamelogic03_player < gamelogic04_player)? GAME_LOGIC_SERVER_THIRD:GAME_LOGIC_SERVER_FOURTH;

	uint8_t ret = (nl->route.online_player[id1] < nl->route.online_player[id2])? id1 : id2;

	return ret;
}

//SOCKET_SUCCESS
static int route_set_gamelogic_id(net_logic* nl,int socket_id)
{
	if(nl->route.sock_id2game_logic[socket_id] == PLAYER_TYPE_INVALID) //新的客户端
	{
		nl->route.game_service_id = route_distribute_gamelogic(nl);    //分配游戏服务来处理
		nl->route.sock_id2game_logic[socket_id] = nl->route.game_service_id; 
		nl->route.online_player[nl->route.game_service_id] ++;
		printf("~~~~ socket is invalid,nl->route.game_service_id = %d ~~~~~\n",nl->route.game_service_id);
		return 0;
	}
	else
	{
		fprintf(ERR_FILE,"route_set_gamelogic_id: this player was used\n");
		return -1;		
	}	
	return 0;
}

//SOCKET_CLOSE
static int route_clear_gamelogic_id(net_logic* nl,int socket_id)
{
	if(nl->route.sock_id2game_logic[socket_id] == PLAYER_TYPE_INVALID)
	{
		fprintf(ERR_FILE,"route_clear_gamelogic_id: a error socket_id\n");
		return -1;			
	}
	else
	{
		nl->route.sock_id2game_logic[socket_id] = PLAYER_TYPE_INVALID;
		nl->route.online_player[nl->route.game_service_id] --;
	}
	return 0;
}

//get game_logic by socket_id
static int route_get_gamelogic_id(net_logic* nl,int socket_id)
{
	if(nl->route.sock_id2game_logic[socket_id] != PLAYER_TYPE_INVALID)
	{
		nl->route.game_service_id = nl->route.sock_id2game_logic[socket_id];
		return 0;
	}
	else
	{
		fprintf(ERR_FILE,"route_get_gamelogic_id: PLAYER_TYPE_INVALID\n");
		return -1;			
	}
	return -1;
}

static inline 
queue* route_get_msg_que(net_logic* nl,int socket_id)
{
	if(route_get_gamelogic_id(nl,socket_id) == 0)
		return nl->route.que_2_gamelogic[nl->route.game_service_id];
	else
		return NULL;
}

static inline 
int route_get_msg_socket(net_logic* nl,int socket_id)
{
	return nl->route.gamelog_socket[nl->route.game_service_id];		   //socket
}

//这个函数看看能不能改一下，这个函数必须依赖网络IO线程按照指定格式读，这样效率低
static deserialize* unpack_user_data(unsigned char * data_pack,int len)
{
	unsigned char proto_type = data_pack[HEAD_PROTO_TYPE_INDEX]; 	 //记录用的.proto文件中哪个message来序列化  
	unsigned char* seria_data = data_pack + PROTO_TYPE_SIZE;      //data_pack 是网络IO线程中分配的内存，反序列化完之后free掉
	deserialize* data = (deserialize*)malloc(sizeof(deserialize));
	int data_len = len - PROTO_TYPE_SIZE; //减去第一个字节包头
	void * msg = NULL;
	switch(proto_type)
	{
		case LOG_REQ:
			msg = login_req__unpack(NULL,data_len,seria_data);//函数里面malloc了内存，返回给msg使用
			printf("nelogic service: client login require,upack data is:%s\n",((login_req*)msg)->name);
			break;		

		case HERO_MSG_REQ:
			msg = hero_msg__unpack(NULL,data_len,seria_data);
			break;			

		case GAME_START_REQ:
			msg = start_req__unpack(NULL,data_len,seria_data);
			printf("nelogic service: client game start require\n");
			break;				 

		// case HEART_REQ:
		// 	msg = heart_beat__unpack(NULL,data_len,seria_data);
		// 	break;			
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
		sv->service_id = 0;
		sv->sock_fd = 0;
		sv->type = 0;
	}

	nt->check_que = false;
	nt->event_index = 0;
	nt->event_n = 0;

	nt->service_id = start->service_id;
	nt->que_pool = start->que_pool;

	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_FIRST]  = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_FIRST];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_SECOND] = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_SECOND];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_THIRD]  = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_THIRD];
	nt->route.que_2_gamelogic[GAME_LOGIC_SERVER_FOURTH] = &nt->que_pool[QUE_ID_NETLOGIC_2_GAME_FOURTH];

	for(int i=0; i<MAX_SOCKET; i++)
	{
		nt->route.sock_id2game_logic[i] = PLAYER_TYPE_INVALID;//未使用的空间，这个socket未连接上
	}
	for(int i=0; i<GAME_LOGIC_SERVICE_NUM; i++)
	{
		nt->route.online_player[i] = 0;
	}

	nt->serv_addr = start->service_addr;
	nt->serv_port = start->service_port;
	nt->network_que = &nt->que_pool[QUE_ID_NETLOGIC_2_NETIO];
	return nt;
}

net_logic_start* net_logic_start_creat(queue* que_pool,configure* conf,int service_id)
{
	net_logic_start* start = malloc(sizeof(net_logic_start));
	if(start == NULL)
	{
       fprintf(ERR_FILE,"net_logic_start_creat:start malloc failed\n");
       return NULL;			
	}

	start->que_pool = que_pool;
	start->service_id = service_id;
	start->netio_addr = conf->service_address;
	start->netio_port = conf->service_port;

	start->service_addr = conf->service_address;
	start->service_port = conf->service_route_port; //监听的 address 和 socket

	return start;
}

static int send_msg_2_game_logic(net_logic* nl,q_node* qnode,int uid) //socket_id
{
	queue* que = route_get_msg_que(nl,uid);
	if(que == NULL)
	{
		fprintf(ERR_FILE,"send_msg_2_game_logic:a null queue\n");
		return -1;			
	}
	queue_push(que,qnode);
	int socket = route_get_msg_socket(nl,uid);

	if(send_msg2_service(socket) == -1)
	{
		fprintf(ERR_FILE,"send_msg_2_game_logic:send_msg2_service failed\n");
		return -1;		
	}
	return 0;
}

static q_node* pack_user_data(deserialize* desseria_data,int uid_pack,char type_pack)
{
	msg_head* head = msg_head_create(type_pack,desseria_data->proto_type,uid_pack,INVALID);
	if(head == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:head malloc failed\n");
		return NULL;
	}
	q_node* qnode = qnode_create(head,desseria_data->buffer,NULL);
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:qnode malloc failed\n");
		return NULL;
	}	
	return qnode;
}


static q_node* pack_inform_data(int uid_pack,char type_pack)
{
	msg_head* head = msg_head_create(type_pack,INVALID,uid_pack,INVALID);
	if(head == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:head malloc failed\n");
		return NULL;
	}

	q_node* qnode = qnode_create(head,NULL,NULL);
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"pack_user_data:qnode malloc failed\n");
		return NULL;
	}		
	return qnode;
}

static int dispose_netio_service_que(net_logic* nl,q_node* qnode)
{
	msg_head* head = (msg_head*)qnode->msg_head;
	char type = head->msg_type;  		// 'D' 'S' 'C'
	int uid = head->uid; 				
	int content_len = head->len; 		//数据长度 = 内容长度 + 1字节的打包类型	
	unsigned char* data = qnode->buffer;			
	
	printf("-----dispose_netio_service_que,type = %c -------\n",type);
	switch(type)
	{
		case TYPE_DATA:     			// 'D' 
		{
			printf("^^^^^^ netlogic type = TYPE_DATA ^^^^^^^^^^^\n");
			deserialize* deseria_data = unpack_user_data(data,content_len);    	//upack
			q_node* send_qnode = pack_user_data(deseria_data,uid,type);        	//pack
			send_msg_2_game_logic(nl,send_qnode,uid);							//send
			break;				
		}

		case TYPE_CLOSE:    			// 'C'
		{
			printf("^^^^^^ netlogic type = TYPE_CLOSE ^^^^^^^^^^^\n");
			q_node* send_qnode = pack_inform_data(uid,type); 
			send_msg_2_game_logic(nl,send_qnode,uid);
			route_clear_gamelogic_id(nl,uid);  									//清空路由表对应的socket位置
			break;
		}
		
		case TYPE_SUCCESS:  			// 'S' 
		{
			printf("^^^^^^ netlogic type = TYPE_SUCCESS ^^^^^^^^^^^\n");
			route_set_gamelogic_id(nl,uid);
			q_node* send_qnode = pack_inform_data(uid,type); 
			send_msg_2_game_logic(nl,send_qnode,uid);
			break;				
		}
	}	
	return 0;	
} 


uint8_t* log_rsp_data_pack(void* pack_data,int* len)
{
	int pack_size = login_rsp_get_packed_size(pack_data);
	uint8_t* out_buf = (uint8_t*)malloc(pack_size + PROTO_HEAD_SIZE);
	if(out_buf == NULL)
		return NULL;

	login_rsp_pack(pack_data,out_buf+PROTO_HEAD_SIZE);
	out_buf[HEAD_PROTO_TYPE_INDEX] = pack_size + 1;
	out_buf[HEAD_PROTO_SIZE_INDEX] = LOG_RSP;

	*len = pack_size + PROTO_HEAD_SIZE;

	return out_buf;
}

uint8_t* enemy_msg_data_pack(void* pack_data,int* len)
{
	int pack_size = enemy_msg_get_packed_size(pack_data);
	uint8_t* out_buf = (uint8_t*)malloc(pack_size + PROTO_HEAD_SIZE);
	if(out_buf == NULL)
		return NULL;

	enemy_msg_pack(pack_data,out_buf+PROTO_HEAD_SIZE);
	out_buf[HEAD_PROTO_TYPE_INDEX] = pack_size + 1;
	out_buf[HEAD_PROTO_SIZE_INDEX] = ENEMY_MSG;	

	*len = pack_size + PROTO_HEAD_SIZE;

	return out_buf;
}

uint8_t* start_rsp_data_pack(void* pack_data,int* len)
{
	int pack_size = start_rsp_get_packed_size(pack_data);
	uint8_t* out_buf = (uint8_t*)malloc(pack_size + PROTO_HEAD_SIZE);
	if(out_buf == NULL)
		return NULL;

	start_rsp_pack(pack_data,out_buf+PROTO_HEAD_SIZE);
	out_buf[HEAD_PROTO_TYPE_INDEX] = pack_size + 1;
	out_buf[HEAD_PROTO_SIZE_INDEX] = GAME_START_RSP;	

	*len = pack_size + PROTO_HEAD_SIZE;

	return out_buf;
}

uint8_t* new_enemy_data_pack(void* pack_data,int* len)
{
	int pack_size = new_enemy_get_packed_size(pack_data);
	uint8_t* out_buf = (uint8_t*)malloc(pack_size + PROTO_HEAD_SIZE);
	if(out_buf == NULL)
		return NULL;

	new_enemy_pack(pack_data,out_buf+PROTO_HEAD_SIZE);
	out_buf[HEAD_PROTO_TYPE_INDEX] = pack_size + 1;
	out_buf[HEAD_PROTO_SIZE_INDEX] = NEW_ENEMY;	

	*len = pack_size + PROTO_HEAD_SIZE;

	return out_buf;
}

uint8_t* login_end_data_pack(void* pack_data,int* len)
{
	int pack_size = login_end_get_packed_size(pack_data);
	uint8_t* out_buf = (uint8_t*)malloc(pack_size + PROTO_HEAD_SIZE);
	if(out_buf == NULL)
		return NULL;

	login_end_pack(pack_data,out_buf+PROTO_HEAD_SIZE);
	out_buf[HEAD_PROTO_TYPE_INDEX] = pack_size + 1;
	out_buf[HEAD_PROTO_SIZE_INDEX] = LOGIN_END;	

	*len = pack_size + PROTO_HEAD_SIZE;

	return out_buf;	
}

//这个函数要修改 qnode 部分
//这个处理函数功能就是:
//对数据根据proto_type进行protobuf的序列化->打包成 proto_type + len + seria_data 数据 ->push到socket的发送服务
static int dispose_game_service_que(net_logic* nl,q_node* qnode)
{
	game_msg_head* msg_head = (game_msg_head*)qnode->msg_head;
	game_msg_data* msg_data = (game_msg_data*)qnode->buffer;  
	char proto_type = msg_data->proto_type;
	void* buffer = msg_data->buffer;
	void* rsp = NULL;
	int rsp_size = 0;
	
	switch(proto_type)
	{
		case LOG_RSP:
			printf("\n\n\n\n^^^^^^net route:LOG_RSP^^^^^^^^\n\n\n");
			rsp = log_rsp_data_pack(buffer,&rsp_size);
			break;			

		case ENEMY_MSG:
			printf("\n\n\n\n^^^^^^net route:ENEMY_MSG^^^^^^^^\n\n\n\n\n");
			rsp = enemy_msg_data_pack(buffer,&rsp_size);
			break;

		case LOGIN_END:
			printf("\n\n\n\n^^^^^^net route:LOGIN_END^^^^^^^^\n\n\n\n\n");
			rsp = login_end_data_pack(buffer,&rsp_size);
			break;

		case GAME_START_RSP:
			printf("\n\n\n\n^^^^^^net route:GAME_START_RSP^^^^^^^^\n\n\n\n");
			rsp = start_rsp_data_pack(buffer,&rsp_size);
			break;

		case NEW_ENEMY:
			printf("\n\n\n\n^^^^^^net route:NEW_ENEMY^^^^^^^^\n\n\n\n\n");
			rsp = new_enemy_data_pack(buffer,&rsp_size);
			break;
	}

	//pack to qnode -> send to write
	msg_head->size = rsp_size;	//记录发送的数据的长度,用于发送
	q_node* node = qnode_create(msg_head,rsp,NULL);
	if(node == NULL)
	{
		fprintf(ERR_FILE,"dispose_game_service_que:qnode malloc failed\n");
		return -1;
	}	

	queue_push(nl->network_que,node);
	send_msg2_service(nl->netio_netroute_socket);	//通知netio服务处理
	return 0;
}

static int dispose_queue_event(net_logic* nl)
{
	int service_type = nl->current_serice->type;
	q_node* qnode = queue_pop(nl->current_que);
	if(qnode == NULL) //队列无数据
	{
		return -1;
	}
	else
	{
		switch(service_type)
		{
			case SERVICE_TYPE_NET_IO:			
				dispose_netio_service_que(nl,qnode);
				break;

			case SERVICE_TYPE_GAME_LOGIC:
				dispose_game_service_que(nl,qnode);
				printf("\n\n\n~~~~~~netlogic: dispose game queue~~~~~~~~~~\n\n");
				break;

			case SERVICE_TYPE_LOG:
				break;

			case SERVICE_TYPE_CHAT:
				break;	
		}

		if(qnode != NULL)
			free(qnode);		
	}
	return 0;
}

static int dispose_service_read_msg(net_logic* nt,service* sv)
{
	char buf[64] = {0};
	int n = read(sv->sock_fd,buf,sizeof(buf));  //写到了这里接着写下去
	buf[n] = '\0';
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
	printf("netlogic read:%s\n",buf);
	return 0;
}

static int net_logic_event(net_logic *nt)
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
					int type = dispose_service_read_msg(nt,sv);
					printf("SERVICE_TYPE = SERVICE_TYPE_NET_IO,netlogic epoll work\n");
					return type;
				}
				break;

			case SERVICE_TYPE_GAME_LOGIC: 	//游戏逻辑处理进程，去处理广播信息		
				nt->current_que = &nt->que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
				nt->current_serice = sv;
				nt->check_que = true;
				if(eve->read)
				{
					int type = dispose_service_read_msg(nt,sv);
					printf("SERVICE_TYPE = SERVICE_TYPE_GAME_LOGIC,netlogic epoll work\n");
					return type;
				}
				break;
		}
	}
}

static int netlogic_lisen_creat(net_logic* nl)
{
    int listen_fd;
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd == -1)
    {
        fprintf(ERR_FILE,"listen socket create error\n");
        return -1;
    }

    struct sockaddr_in serv_addr;     //ipv4 struction
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   //ipv4
    serv_addr.sin_addr.s_addr = inet_addr(nl->serv_addr);
    serv_addr.sin_port = htons(nl->serv_port);      //主机->网络 nl->serv_port

    int optval = 1;
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval)) == -1)
    {
    	fprintf(ERR_FILE,"setsockopt failed\n");  
    	goto _err;   
    }
    if (bind(listen_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1)
    {
        fprintf(ERR_FILE,"bind failed\n");  
        goto _err;      
    }
	if (listen(listen_fd, MAX_SERVICE) == -1) 
	{
		fprintf(ERR_FILE,"listen failed\n"); 
		goto _err;
	}   
	nl->listen_fd = listen_fd;
    return 0;

_err:
	close(listen_fd);
	return -1;  	
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

    memset(&netlog_service_addr,0,sizeof(netlog_service_addr));
    netlog_service_addr.sin_family = AF_INET;
    netlog_service_addr.sin_port = htons(PORT_NETROUTE_2_NETIO_SERVICE);		

	int optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		perror("setsockopt bind\n");
		return -1; 
	}

    if(bind(sockfd,(struct sockaddr*)(&netlog_service_addr),sizeof(netlog_service_addr)) == -1)
    {
       fprintf(ERR_FILE,"connect_netio_service:bind failed\n");
       perror("netlogic bind err");
       return -1;    	
    }
    //set service address
    memset(&netio_server_addr,0,sizeof(netio_server_addr));
    netio_server_addr.sin_family = AF_INET;
    netio_server_addr.sin_port = htons(netio_port);
    netio_server_addr.sin_addr.s_addr = inet_addr(netio_addr);

    for( ;; )
    {
	    if((connect(sockfd,(struct sockaddr*)(&netio_server_addr),sizeof(struct sockaddr))) == -1)
	    {
	        fprintf(ERR_FILE,"netlogic:connect_netio_service:netlogic_thread disconnect\n");
	        sleep(1);
	    }
	    else
	    {
		    //connect sussess
		    service* sv = &nl->service_pool[SERVICE_ID_NETWORK_IO];
		    sv->service_id = SERVICE_ID_NETWORK_IO;
		    sv->sock_fd = sockfd;
		    sv->type = SERVICE_TYPE_NET_IO;
		    nl->netio_netroute_socket = sockfd;

		    //add to epoll
		    printf("netlogic:net_logic service connect to net_io service success!\n");
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


static int accept_gamelog_service_connect(net_logic* nl)
{	
 	struct sockaddr_in addr; 	   //tcpip地址结构
	socklen_t addr_len = sizeof(addr);
	int listen_fd = nl->listen_fd;
	int socket = 0;
	int accept_num = 0;
	for( ; ; )
	{
		socket = accept(listen_fd,(struct sockaddr *)&addr, (socklen_t *)&addr_len);
		if (socket == -1)
		{
			fprintf(ERR_FILE,"wait_netlogic_thread_connect: socket connect failed\n");
			return -1;
		}   
		else
		{
			int port = ntohs(addr.sin_port);  //客户端的端口
			printf("netlogic dispatch accept,port = %d\n",port);
			switch(port)
			{
				case PORT_GAME_LOGIC_SERVICE_FIRST:
					nl->route.gamelog_socket[GAME_LOGIC_SERVER_FIRST] = socket;							
					accept_num++;
					break;

				case PORT_GAME_LOGIC_SERVICE_SECOND:
					nl->route.gamelog_socket[GAME_LOGIC_SERVER_SECOND] = socket;
					accept_num++;				
					break;

				case PORT_GAME_LOGIC_SERVICE_THIRD:
					nl->route.gamelog_socket[GAME_LOGIC_SERVER_THIRD] = socket;
					accept_num++;
					break;

				case PORT_GAME_LOGIC_SERVICE_FOURTH:
					nl->route.gamelog_socket[GAME_LOGIC_SERVER_FOURTH] = socket;
					accept_num++;
					break;

				default:
					printf("port error\n");
					close(socket); 
					break;					
			}	
			if(accept_num == GAME_LOGIC_SERVICE_NUM)
			{
				for(int i=0; i<GAME_LOGIC_SERVICE_NUM; i++)
				{
					service* sv = &(nl->service_pool[SERVICE_ID_GAME_FIRST+i]);
					sv->service_id = SERVICE_ID_GAME_FIRST + i;
					sv->sock_fd = nl->route.gamelog_socket[i];
					sv->type = SERVICE_TYPE_GAME_LOGIC;
					int sockfd = sv->sock_fd;

					socket_keepalive(sv->sock_fd);
					if(set_nonblock(sv->sock_fd) == -1)
					{
						fprintf(ERR_FILE,"set set_nonblock failed\n");
						return -1;
					}
					if(epoll_add(nl->epoll_fd,sockfd,sv) == -1)
			    	{
			       		fprintf(ERR_FILE,"accept_gamelog_service_connect:epoll_add failed\n");
			       		return -1;    	
			    	} 
				}	
				printf("connect all game_logic service\n");
				return 0;
			}
		}		
	}
	return -1;
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
	if(accept_gamelog_service_connect(nl) == -1)
	{
       fprintf(ERR_FILE,"service_connect_establish:connect game_logic service failed\n");
       return -1;			
	}
	return 0;
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
	if(netlogic_lisen_creat(nt) == -1)
	{
       fprintf(ERR_FILE,"net_logic_service_loop:netlogic_lisen_creat\n");
       return NULL;			
	}
	if(service_connect_establish(start,nt) == -1)
	{
       fprintf(ERR_FILE,"net_logic_service_loop:service_connect_establish failed\n");
       return NULL;			
	}
	int type = 0;
	printf("~~~~~~~~~~net_logic_service_loop running!~~~~~~~~~~~~~\n");
	for( ; ; )
	{
		type = net_logic_event(nt);
		switch(type)
		{
			case EVENT_TYPE_QUE_NULL:
				printf("netlogic:queue is null\n");
				break;

			case EVENT_THREAD_CONNECT_ERR:
				break;

			case EVENT_THREAD_DISCONNECT:
				break;
		}
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


//测试 client -> netio -> net_logic 打印反序列化之后的数据 
void send_data_test(net_logic* nl,queue* que,q_node* qnode)
{	
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"send_data_test:qnode is null\n");
		return ;		
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
	for(int i=0;i<4;i++)
	{
		int socket = nl->route.gamelog_socket[i];
		char* buf = "DATA";
		int n = write(socket,buf,strlen(buf));
		if(n == strlen(buf))
		{
			printf("send data success\n");
		}
	}
	
}


 */