/*
网络 IO 的核心部分代码
*/
#include "socket_server.h"
#include "socket_epoll.h"
#include "lock_queue.h"
#include "game_logic.h"
#include "err.h"
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

#define DATA_LEN_SIZE   			  1                 //1个字节存储长度信息
#define MAX_EVENT 					 64                //epoll_wait一次最多返回64个事件

#define SOCKET_READBUFF 		    128
#define PIPE_HEAD_BUFF    			128   
#define MAXPIPE_CONTENT_BUFF    	128     
#define MAX_BACK_LOG 				 32

#define SOCKET_TYPE_INVALID          1		   
#define SOCKET_TYPE_LISTEN_NOTADD    2		
#define SOCKET_TYPE_LISTEN_ADD       3		
#define SOCKET_TYPE_CONNECT_ADD      4	    
#define SOCKET_TYPE_HALFCLOSE        5	    
#define SOCKET_TYPE_CONNECT_NOTADD   6	
#define SOCKET_TYPE_OTHER            7		
#define SOCKET_TYPE_PIPE_READ        8
#define SOCKET_TYPE_PIPE_WRITE       9
#define SOCKET_TYPE_NETLOGIC		10

struct append_buffer
{
	struct append_buffer* next;
	void* buffer;     	//in order to free memery
	void* current;
	int size;        	//这块缓冲区中剩余未发送的字节数
};

struct socket
{
	int fd;          			//socket fd  
	int id;          			//id
	int type;     	 			//socket type
	int remain_size; 			//缓冲区链表剩余的字节数
	struct append_buffer* head;
	struct append_buffer* tail;
};

struct socket_server 
{
    int epoll_fd;                //epoll fd
    int event_n;                 //epoll_wait return number of event
    struct socket* socket_pool;  //socket pool，record every socket massage
    struct event* event_pool;    //event pool,record event massage
    int event_index;             //from 0 to 63
    int pipe_read_fd;
    int pipe_write_fd;
    queue* io2netlogic_que;
    queue* netlogic2io_que;
    int service_id;
    char* address;
    int port;
    int listen_fd;
    int alloc_id;				  //用于记录本次分配了的最后一个id
    struct socket* socket_netlog;
    bool que_check;               //检测消息队列的标志位
};

//------------------------------------------------------------------------------------------------------------------
static int append_remaindata(struct socket *s,void* buf,int buf_size,int start)
{
	struct append_buffer* node = (struct append_buffer*)malloc(sizeof(struct append_buffer));
	if(node == NULL)
		return -1;

	char* append_buf = (char*)malloc(buf_size);
	if(append_buf == NULL)
		return -1;

	strncpy(append_buf,(char*)buf,buf_size);
	node->current = append_buf + start;
	node->size = buf_size - start;
	node->buffer = append_buf;
	node->next = NULL;
	s->remain_size += node->size;

	if(s->head == NULL)
	{
		s->head = s->tail = node;
	}
	else
	{
		s->tail->next = node;
		s->tail = node;
	}
	return 0;
}

//id from 1-2^31-1
static int apply_id(struct socket_server *ss)
{
	struct socket* s = NULL;
	int last_id = ss->alloc_id;

	if(last_id >= (MAX_SOCKET-1)) 
	{
		ss->alloc_id = 0;
	}
	for(int id=last_id+1; id<MAX_SOCKET; id++)
	{
		s = &ss->socket_pool[id % MAX_SOCKET];
		if(s->type == SOCKET_TYPE_INVALID)
		{
			ss->alloc_id = id;
			return id;
		}
	}
	return -1;
}

//apply a socket from socket_pool 
static struct socket* apply_socket(struct socket_server *ss,int fd,int id,bool add_epoll)
{
	struct socket* s = &ss->socket_pool[id % MAX_SOCKET];
	if(s == NULL)
	{
	 	return NULL;
	}

	printf("---------------------------------------------id = %d\n",id);
	printf("---------------------------------------------id / MAX_SOCKET = %d\n",id % MAX_SOCKET);
	printf("---------------------------------------------s->type = %d\n",s->type);

	assert(s->type == SOCKET_TYPE_INVALID);

	if(add_epoll)
	{
		if(epoll_add(ss->epoll_fd,fd,s) == -1)
		{
			s->type = SOCKET_TYPE_INVALID;
			return NULL;
		}
	}
	s->fd = fd;
	s->id = id;
	s->remain_size = 0;
	s->head = NULL;
	s->tail = NULL;
	return s;
}

void socket_keepalive(int fd)
{
	int keep_alive = 1;
	setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,(void*)&keep_alive,sizeof(keep_alive));
}

static int do_listen(const char* host,int port,int max_connect)
{
    int listen_fd;
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd == -1)
    {
        fprintf(ERR_FILE,"listen socket create error\n");
        return -1;
    }

    struct sockaddr_in serv_addr;     	//ipv4 struction
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  	//ipv4
    serv_addr.sin_addr.s_addr = inet_addr(host);
    serv_addr.sin_port = htons(port);              //主机->网络
    printf(":::::::network io service address = %s listen port = %d:::::::::\n",host,port);

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
	if (listen(listen_fd, max_connect) == -1) 
	{
		fprintf(ERR_FILE,"listen failed\n"); 
		goto _err;
	}    
    return listen_fd;

_err:
	close(listen_fd);
	return -1;  
}

//为 listen_fd 申请 socket_pool 中一个成员
static int listen_socket(struct socket_server *ss,int listen_fd,int id)
{
	struct socket *s = apply_socket(ss,listen_fd,id,false);
	if(s == NULL)
	{
		fprintf(ERR_FILE,"listen_id apply socket failed\n"); 
		goto _err;
	}  
	s->type = SOCKET_TYPE_LISTEN_NOTADD;//未放入 epoll 中管理
	return 0;
_err:
	close(listen_fd);
	ss->socket_pool[id % MAX_SOCKET].type = SOCKET_TYPE_INVALID; //return to pool
	return -1;
}

static int dispose_accept(struct socket_server *ss,struct socket *s,struct socket_message *result)
{
	int listen_fd = s->fd;
	struct sockaddr_in address;
	socklen_t addr_len = sizeof(address);
	int client_fd = accept(listen_fd,(struct sockaddr*)&address,(socklen_t*)&addr_len);
	if(client_fd == -1)
	{
		fprintf(ERR_FILE,"accept failed\n");
		return -1;
	}

	int id = apply_id(ss);
	socket_keepalive(client_fd);
	if(set_nonblock(client_fd) == -1)
	{
		fprintf(ERR_FILE,"set set_nonblock failed\n");
		return -1;
	}
	struct socket* cs = apply_socket(ss,client_fd,id,false);
	if(cs == NULL) //
	{
		close(client_fd);
		fprintf(ERR_FILE,"apply socket from pool failed\n");
		return -1;
	}
	cs->type = SOCKET_TYPE_CONNECT_NOTADD;
	result->id = id;
	result->lid_size = s->id; //listen_id
	result->data = "accept new client\n";
	return 0;   
}

//这个函数要修改，增加内存泄露管理，已修改
static void close_fd(struct socket_server *ss,struct socket *s,struct socket_message * result)
{
	if(s->type == SOCKET_TYPE_INVALID)
	{
		return;
	}
	if(s->type == SOCKET_TYPE_LISTEN_ADD || s->type == SOCKET_TYPE_CONNECT_ADD) //
	{
		if(epoll_del(ss->epoll_fd,s->fd) == -1)
		{
			fprintf(ERR_FILE,"epoll_del failed s->fd=%d\n",s->fd);
		}
	}
	if(result != NULL)
	{
		result->id = s->id;
		result->data = "close\n";		
	}

	
	struct append_buffer* tmp = s->head;
	while( tmp ) //free
	{
		s->head = s->head->next;
		free(tmp->buffer);
		free(tmp);
		tmp = s->head;
	}
	close(s->fd);

	printf("^^^^^^^^^close id = %d^^^^^^^^^^^^\n",s->id);
	s->type = SOCKET_TYPE_INVALID;
	s->id = 0;
	s->head = NULL;
	s->tail = NULL;
	s->remain_size = 0;
}


//处理epoll的可读事件
//这个函数还需要改，如果第二次读len长度数据时候被信号中断了改怎么办
//s中再增加一个成员记录？如果是0则不处理，如果不为0则按这个长度读？
static int dispose_readmessage(struct socket_server *ss,struct socket *s, struct socket_message * result)
{
	unsigned char len = 0;
	char* buffer  = NULL;
	int n = (int)read(s->fd,&len,DATA_LEN_SIZE);
	if(n <= 0)
		goto _err;
	assert(n == DATA_LEN_SIZE);

	buffer = (char*)malloc(len);  
	if(buffer == NULL) 
	{
		fprintf(ERR_FILE,"dispose_readmessage: result->buffer is NULL\n");
		return -1;
	}
	memset(buffer,0,len);

	n = (int)read(s->fd,buffer,len);

	if(n <= 0)
		goto _err;

	assert(n == len);

	result->id = s->id;
	result->lid_size = n;
	result->data = buffer;  
	return SOCKET_DATA;	

_err:	
	if(n < 0)
	{
		if(buffer != NULL)
		{
			free(buffer);
			buffer = NULL;
		}
		switch(errno)
		{
			case EINTR:
				fprintf(ERR_FILE,"dispose_readmessage: socket read,EINTR\n");
				break;    	// wait for next time
			case EAGAIN:		
				fprintf(ERR_FILE,"dispose_readmessage: socket read,EAGAIN\n");
				break;
			default:
				printf("\n\n\n\n<<~~~~~~~SOCKET_ERROR~~~~~~~~~~>>\n\n\n\n\n");
				perror("SOCKET_ERROR");
				close_fd(ss,s,result);
				report_socket_error(ss,s->id);

				return SOCKET_ERROR;			
		}
	}
	if(n == 0) //client close,important
	{
		 if(buffer != NULL)
		 {
		 	free(buffer);
		 	buffer = NULL;
		 }
		 printf("\n\n\n\n<<~~~~~~~~~~client close~~~~~~~~~>>\n\n\n\n\n");
		 close_fd(ss,s,result);
		 return SOCKET_CLOSE;
	}
	return -1;
}


static void send_client_msg2net_logic(struct socket_server* ss,q_node* qnode)
{
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"send_client_msg2net_logic:a null qnode\n");
		return; 		
	}
	queue_push(ss->io2netlogic_que,qnode); //封装成一个send函数

	int socket = (ss->socket_netlog)->fd;
	send_msg2_service(socket);			   //发送通知给 net_logic service
}


static void report_socket_error(struct socket_server* ss,int uid)
{
	msg_head* head = msg_head_create(TYPE_CLOSE,INVALID,uid,INVALID);
	q_node* qnode = qnode_create(head,NULL,NULL);			//
	send_client_msg2net_logic(ss,qnode);         	//通知 netlogic service
}


//把应用层缓冲区中的数据发送出去
static int send_buffer(struct socket_server* ss,struct socket *s,struct socket_message *result)
{
	while(s->head)
	{ 
		struct append_buffer * tmp = s->head;
		for( ; ; )
		{
			int n = write(s->fd,tmp->current,tmp->size);
			if(n == -1)
			{
				switch(errno)
				{
					case EINTR:
						continue;
					case EAGAIN:
						return -1;
					default:
					fprintf(ERR_FILE, "send_data: write to %d (fd=%d) error.",s->id,s->fd);
					close_fd(ss,s,result);//还需呀通知游戏服务逻辑这个id关闭了
					report_socket_error(ss,s->id);
					return -1;
				}
			}
			s->remain_size -= n;
			if(n != tmp->size)   //未完全发送完
			{
				tmp->current += n;
				tmp->size -= n; 
			}
			if(n == tmp->size)  //这一块数据已经发送完成
			{
				s->head = tmp->next;    
				if(tmp->buffer != NULL)
				{
					free(tmp->buffer);
					free(tmp); 
				}				
				break;
			}
		}	
	}	
	s->tail = NULL;
	epoll_write(ss->epoll_fd,s->fd,s,false);  //写完了，取消关注写事件
	return 0;
}



// 负责单个socket成员数据的发送
// 如果缓冲区满了，那么就拷贝内存，然后把数据追加到 s 的缓冲区中
static int send_data(struct socket_server* ss,struct socket *s,void* buf,int len)
{
	assert(s->type == SOCKET_TYPE_CONNECT_ADD);  //必须是加入到epoll管理的socket才能发送数据

	if(s->head == NULL) //追加的缓冲区中不存在未发送的数据
	{
		int n = write(s->fd,buf,len);
		if(n == -1)
		{
			switch(errno)
			{
				case EINTR:
				case EAGAIN:
					n = 0;
					break;
				default:
					fprintf(ERR_FILE, "************socket_server_send: write to fd=%d error.************",s->fd);			
					close_fd(ss,s,NULL);
					report_socket_error(ss,s->id);
					return -1;
			}
		}
		if(n == len) //send success
		{		
			printf("^^^^^^n=len=%d\n",n);
			return 0;
		}
		if(n < len)  //send buffer full
		{
			printf("############append data###################\n");
			append_remaindata(s,buf,len,n); 		 	//append to buffer
			epoll_write(ss->epoll_fd,s->fd,s,true);
		}		
	}
	else
	{
		append_remaindata(s,buf,len,0);
	}

	return 0;		
}


static int broadcast_user_msg(struct socket_server* ss,q_node* qnode)
{
	game_msg_head* msg_head = (game_msg_head*)qnode->msg_head;
	int socket_num = msg_head->broadcast_player_num;
	int *uid_list = msg_head->uid_list;
	int request_len = msg_head->size;
	void* msg_buf = qnode->buffer;
	printf("--------socket_num = %d------------\n",socket_num);
	
	for(int i=0; i<socket_num; i++)
	{
		int id = uid_list[i];
		printf("^^^^^^^^^^^netio:broadcast to uid = %d^^^^^^^^^^^^^^^^^^\n",id);
		struct socket *s = &ss->socket_pool[id % MAX_SOCKET];
		send_data(ss,s,msg_buf,request_len);
		printf(">>>>>>>>>send data end<<<<<<<<<<<<<<<<\n");
	}

	if(msg_head != NULL)
		free(msg_head);
	if(msg_buf != NULL)
		free(msg_buf);

	return 0;	
}

//发送通知唤醒其他服务的函数
int send_msg2_service(int socket)
{
	char* buf = "D"; 	//无任何意义的数据，为了唤醒其他服务
	int n = write(socket,buf,strlen(buf));
	if(n == -1)
	{
		switch(errno)
		{
			case EINTR:
				//continue;
			case EAGAIN:
				return 0; //wait next time
			default:
				close(socket);
				fprintf(stderr, "send_msg2_service: write socket = %d error.",socket);
				return -1;
		}
	}
	return 0;	
}


//----------------------------------------------------------------------------------------------------------------------
static struct socket_server* socket_server_create(net_io_start* start)
{
	int efd = epoll_init();
	if(efd_err(efd))
	{
		fprintf(ERR_FILE,"socket_server_create:epoll create failed\n");
		return NULL;
	}
	struct socket_server *ss = malloc(sizeof(*ss));
	printf("sizeof(*ss) = %ld,sizoeof(struct socket_server) = %ld\n",sizeof(*ss),sizeof(struct socket_server));
	ss->epoll_fd = efd;
	ss->event_n = 0;
	ss->event_index = 0;
	ss->que_check = false;
	ss->socket_pool = (struct socket*)malloc(sizeof(struct socket)*MAX_SOCKET);

	if(ss->socket_pool == NULL)
	{
		fprintf(ERR_FILE,"socket_server_create:socket_pool malloc failed\n");
		epoll_release(efd);
		return NULL;
	}
	ss->event_pool = (struct event*)malloc(sizeof(struct event)*MAX_EVENT);
	if(ss->event_pool == NULL)
	{
		fprintf(ERR_FILE,"socket_server_create:event_pool malloc failed\n");
		epoll_release(efd);
		free(ss->socket_pool);
		return NULL;
	}

	int i = 0;
	struct socket *s = NULL;
	for(i=0; i<MAX_SOCKET; i++)
	{
		s = &ss->socket_pool[i];
		s->fd = 0;
		s->id = 0;
		s->type = SOCKET_TYPE_INVALID;
		s->remain_size = 0;
		s->head = NULL;
		s->tail = NULL;
	}
	ss->service_id = start->service_id;
	ss->io2netlogic_que = &start->que_pool[QUE_ID_NETIO_2_NETLOGIC];
	ss->netlogic2io_que = &start->que_pool[QUE_ID_NETLOGIC_2_NETIO];
	ss->address = start->address;
	ss->port = start->port;
	ss->alloc_id = 0;

	return ss;
}

static int socket_server_listen(struct socket_server *ss,const char* host,int port,int backlog)
{
	int listen_fd = do_listen(host,port,backlog);
	if(listen_fd == -1)
	{
	   return -1;
	}
	ss->listen_fd = listen_fd;
	int id = apply_id(ss);
	if(listen_socket(ss,listen_fd,id) == 0)
	{
		return id;
	}
	return -1;
}

static int dispose_queue_event(struct socket_server *ss)
{
	queue* que = ss->netlogic2io_que;
	q_node* qnode = queue_pop(que);
	if(qnode == NULL) //队列无数据
	{
		return -1; 
	}
	else
	{
		//把消息队列的数据的内容部分广播给指定玩家
		printf("/************broadcast data to client*****************/\n");
		broadcast_user_msg(ss,qnode);
		if(qnode != NULL)
		{
			free(qnode);
		}
		return 0;
	}
	return -1;
}

static int socket_server_start(struct socket_server *ss,int id)
{
	struct socket *s = &ss->socket_pool[id % MAX_SOCKET];  

	if(s == NULL)
	{
		return SOCKET_ERROR;
	}
	if(s->type == SOCKET_TYPE_INVALID) 
	{
		return SOCKET_ERROR;
	}
	if (s->type == SOCKET_TYPE_CONNECT_NOTADD || s->type == SOCKET_TYPE_LISTEN_NOTADD) 
	{
		if(epoll_add(ss->epoll_fd,s->fd,s) == -1)
		{
			s->type = SOCKET_TYPE_INVALID;
			return SOCKET_ERROR;
		}
		s->type = (s->type == SOCKET_TYPE_CONNECT_NOTADD) ? SOCKET_TYPE_CONNECT_ADD : SOCKET_TYPE_LISTEN_ADD;//change

		return SOCKET_SUCCESS;	//成功加入到 epoll 中管理。
	}
	return SOCKET_ERROR;
}


static int socket_server_event(struct socket_server *ss, struct socket_message * result)
{
	for( ; ; )
	{	
		if(ss->que_check)
		{
			if(dispose_queue_event(ss) == -1)  //queue null
			{
				ss->que_check = false;
				return -1;
			}
			else  							   //have data
			{
				continue;
			}		
		}
		if(ss->event_index == ss->event_n)  
		{
			ss->event_n = sepoll_wait(ss->epoll_fd,ss->event_pool,MAX_EVENT);

			if(ss->event_n <= 0) //error
			{
				fprintf(ERR_FILE,"socket_server_event:sepoll_wait return error event_n\n");
				ss->event_n = 0;
				return -1;
			}	
			ss->event_index = 0;
		}
		struct event* eve = &ss->event_pool[ss->event_index++];
		struct socket *s = eve->s_p; 
		if(s == NULL)
		{
			fprintf(ERR_FILE,"socket_server_event:a NULL socket\n");
			return -1;		
		}
		switch(s->type) 
		{
			case SOCKET_TYPE_NETLOGIC:
				ss->que_check = true;
				break;					

			case SOCKET_TYPE_LISTEN_ADD: 	//client connect
				if(dispose_accept(ss,s,result) == 0)
				{
					// return SOCKET_ACCEPT;	
					printf("accept[id=%d] from [id=%d]\n",result->id,result->lid_size);
					return socket_server_start(ss,result->id); 
				}
				break;

			case SOCKET_TYPE_INVALID:
				fprintf(ERR_FILE,"socket_server_event:a invalied socket from pool\n");
				break;

			case SOCKET_TYPE_CONNECT_ADD:
				if(eve->read)
				{
					int ret_type = dispose_readmessage(ss,s,result);
					if(ret_type == -1)
					{
						fprintf(ERR_FILE,"socket_server_event:a error ret_type\n");
						break; 
					}
					return ret_type;
				}
				if(eve->write)
				{
					send_buffer(ss,s,result);
				}
				if(eve->error)
				{
					
				}
				break;
		}
	}
}


static void socket_server_release(struct socket_server *ss)
{
	int i = 0;
	struct socket* s = NULL;

	for(i=0; i<MAX_SOCKET; i++)
	{
		s = &ss->socket_pool[i];
		if(s->type == SOCKET_TYPE_CONNECT_ADD || s->type == SOCKET_TYPE_LISTEN_ADD)
		{
			epoll_del(ss->epoll_fd,s->fd);
		}
		close(s->fd);
	}
	epoll_release(ss->epoll_fd);	

	free(ss->socket_pool);
	free(ss->event_pool);
	free(ss);
}


//网络io线程只负责读写和负责通知网关处理线程处理
static q_node* dispose_event_result(struct socket_server* ss,struct socket_message *result,int type)
{
	q_node* qnode = NULL;
	msg_head* head = NULL;
	int uid = result->id;
	char* buf = result->data;
	int len = result->lid_size;	
	printf("socket service: dispose_event_result :socket type = %d\n",type);
	switch(type)
	{
		case SOCKET_DATA:
			printf("*********SOCKET_DATA***********\n");
			head = msg_head_create(TYPE_DATA,INVALID,uid,len);
			qnode = qnode_create(head,buf,NULL);
			printf("netio push data to queue\n");
			break;

		case SOCKET_CLOSE: 
			printf("*********SOCKET_CLOSE***********\n");	
			head = msg_head_create(TYPE_CLOSE,INVALID,uid,INVALID);
			qnode = qnode_create(head,NULL,NULL);
			break;

		case SOCKET_SUCCESS:
			printf("*********SOCKET_SUCCESS***********\n");
			head = msg_head_create(TYPE_SUCCESS,INVALID,uid,INVALID);
			qnode = qnode_create(head,NULL,NULL);
			break;
	}
	if(head == NULL || qnode == NULL)
	{
		fprintf(ERR_FILE,"dispose_event_result:head or qnode malloc error\n");
		return NULL; 		
	}
	return qnode;
}



//这里应该不要传递那么多参数，直接传递读配置文件后返回的指针吧
net_io_start* net_io_start_creat(queue* que_pool,configure* conf,int service_id)
{
	net_io_start* start = (net_io_start*)malloc(sizeof(net_io_start));
	start->que_pool = que_pool;
	start->service_id = service_id;
	start->address = conf->service_address; //这个没错,不需要修改
	start->port = conf->service_port;		

	return start;
}

static int wait_netlogic_service_connect(struct socket_server* ss)
{
 	struct sockaddr_in addr; 	   //tcpip地址结构
	socklen_t addr_len = sizeof(addr);
	int listen_fd = ss->listen_fd;
	int socket = 0;

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
			printf("netio dispatch accept,port = %d\n",port);
			if(port == PORT_NETROUTE_2_NETIO_SERVICE) //必须是这个端口
			{
				printf("netio accept netlogic socket is = %d\n",socket);
				socket_keepalive(socket);
				if(set_nonblock(socket) == -1)
				{
					fprintf(ERR_FILE,"wait_netlogic_service_connect:set set_nonblock failed\n");
					return -1;
				}				
				int id = apply_id(ss);
				struct socket* s = apply_socket(ss,socket,id,true);
				if(s == NULL)
				{
					fprintf(ERR_FILE,"wait_netlogic_thread_connect: apply_socket failed\n");
					return -1;
				}
				s->type = SOCKET_TYPE_NETLOGIC;//标记为与网络逻辑线程通信的socket
				ss->socket_netlog = s;		   //与netlogic通信的socket，记录下来

				printf("netio:accept net_logic service connect!\n");	
				return 0;
			}
			else
			{
				printf("port error\n");
				close(socket); 
			}				
		}		
	}
	return 0;
}


//socket_server thread loop
void* network_io_service_loop(void* arg)
{
	net_io_start* start = (net_io_start*)arg;
	struct socket_server* ss = socket_server_create(start);
	if(ss == NULL)
		return NULL;	

	char* address = start->address; 
	int port = start->port;  		
	int listen_id = socket_server_listen(ss,address,port,MAX_BACK_LOG);

	if(listen_id == -1)
	{
		fprintf(ERR_FILE,"network_io_service_loop: socket_server_listen failed\n");
		return NULL;				
	}
	if(wait_netlogic_service_connect(ss) == -1)
	{
		fprintf(ERR_FILE,"wait_netlogic_thread_connect: apply_socket failed\n");
		return NULL;		
	}

	socket_server_start(ss,listen_id);

	struct socket_message result;
	q_node* qnode = NULL;
	int type = 0;
	printf("~~~~~~~~network_io_service_loop running!~~~~~~~~~\n");
	for ( ; ; )
	{
		type = socket_server_event(ss,&result);
		switch(type)  
		{
			case SOCKET_EXIT:
				goto _EXIT;
				
			case SOCKET_ACCEPT://client connect
				// printf("accept[id=%d] from [id=%d]\n",result.id,result.lid_size);
				// socket_server_start(ss,result.id);  			//add to epoll
				printf("SOCKET_ACCEPT accept \n");
				break;

			case SOCKET_ERROR:

				break;

			case SOCKET_DATA:
			case SOCKET_CLOSE:
			case SOCKET_SUCCESS:
				qnode = dispose_event_result(ss,&result,type);	
				send_client_msg2net_logic(ss,qnode);         	//通知 netlogic service
				break;

			default:
				break;	
		}
	}
_EXIT:
	socket_server_release(ss);	
	return NULL;
}





/*--------------------------------------------------------------------------------------------------------------
void read_test(struct socket_server* ss,int id,const char* data,int size,struct socket_message *result)
{
	struct send_data_req * request = (struct send_data_req*)malloc(sizeof(struct send_data_req));
	request -> id = id;
	request->size = size;
	request->buffer = (char*)data;
	socket_server_send(ss,request,result);
}

struct request_close 
{
	int id;
};

struct request_send
{
	int id;         //4
	int size;	   	//4
	char * buffer;	//4
};

struct request_package
{
	uint8_t header[8];//header[0]->massage type header[1]->massage len
	union
	{
		char buffer[256];
		struct request_send send;
		struct request_close close;	
	}msg;
};




static int dispose_pipe_event(struct socket_server *ss,struct socket_message *result)
{
	uint8_t pipe_head[PIPE_HEAD_BUFF];  //msg 
	if(read_from_pipe(ss,pipe_head,PIPE_HEAD_BUFF) == -1)
	{
		return -1;
	}
	uint8_t type = pipe_head[0];
	uint8_t len = pipe_head[1];

	if(type == 'D')
	{
		struct send_data_req send; //content

		if(len > MAXPIPE_CONTENT_BUFF)
			fprintf(ERR_FILE,"dispose_pipe_event:data too large\n");	
			return -1;
		send.buffer = (char*)malloc(len);  //发送出去的内存，需要在send函数中释放掉申请的内存
		if(read_from_pipe(ss,&send,len) == -1)
		{
			return -1;
		}		
		socket_server_send(ss,&send,result);
		return 0;
	}
	if(type == 'C')
	{
		struct close_req close;
		if(read_from_pipe(ss,&close,len) == -1)
		{
			return -1;
		}
		close_socket(ss,&close,result);
		return 0;
	}
	return -1;
}


//#####
static int read_from_pipe(struct socket_server *ss,void* buffer,int len)
{
	int n = 0;
	for( ; ; )
	{
		n = read(ss->pipe_read_fd,buffer,len);
		if(n < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				fprintf(ERR_FILE, "read_from_pipe: read pipe error %s.",strerror(errno));
				return -1;					
			}
			
		}
		if(n == len)
		{
			return 0;
		}
	}
	fprintf(ERR_FILE, "read_from_pipe: read pipe error,need to read size=%d but result size=%d\n",len,n);
	return -1;
}

static int pipe_init(struct socket_server* ss,int pipe_type)
{
	int pipe_fd[2];
	if(pipe(pipe_fd) == -1)
	{
		return -1;
	}
	if(pipe_type == SOCKET_TYPE_PIPE_READ)
	{
//		close(pipe_fd[1]);
		int id = apply_id();
		printf("pipe init\n");
		struct socket *s = apply_socket(ss,pipe_fd[0],id,true);
		printf("pipe socket add to epoll\n");

		if(s == NULL)
		{
			return -1;
		}
		s->type = SOCKET_TYPE_PIPE_READ;		
		ss->pipe_read_fd = pipe_fd[0];
		ss->pipe_write_fd = pipe_fd[1];
	}
	else  
	{
		close(pipe_fd[0]); 
		ss->pipe_write_fd = pipe_fd[1];
	}
	return pipe_fd[0];
}


static int send_notice2_netlogic_service(struct socket_server* ss)
{
	struct socket *s = ss->socket_netlog;
	int socket = s->fd;
	int socket = (ss->socket_netlog)->fd;
	char* buf = "D"; 	//无任何意义的数据，为了唤醒 net_logic service 的 epoll

	int n = write(socket,buf,strlen(buf));
	if(n == -1)
	{
		switch(errno)
		{
			case EINTR:
				//continue;
			case EAGAIN:
				return -1;
			default:
			fprintf(stderr, "send_notice2_netlogic_service: write to netlogic_socket %d (fd=%d) error.",s->id,s->fd);
			//close_fd(ss,s,result);
			//
			return -1;
		}
	}
	return 0;
}


##########
static int close_socket(struct socket_server *ss,struct close_req *close,struct socket_message * result)
{
	int close_id = close->id;
	struct socket *s = &ss->socket_pool[close_id % MAX_SOCKET];

	if(s == NULL || s->type != SOCKET_TYPE_CONNECT_ADD)
	{
		fprintf(ERR_FILE,"close_socket: close a error socket\n");	
		return -1;
	}
	if(s->remain_size)
	{
		int type = send_data(ss,s,result);
		if(type != -1)
			return type;
	}
	if(s->remain_size == 0)
	{
		close_fd(ss,s,result);
		return SOCKET_CLOSE;
	}
	return -1;
}



///管道中接收到其他进程发过了的写socket操作调用。
static int socket_server_send(struct socket_server* ss,struct send_data_req * request,struct socket_message *result)
{
	int id = request->id;
	struct socket * s = &ss->socket_pool[id % MAX_SOCKET];

	if(s->type != SOCKET_TYPE_CONNECT_ADD) //如果已经关闭了那么在dispose_readmassage函数中会对状态进行改变
	{
		if(request->buffer != NULL)
		{
			free(request->buffer);
			request->buffer = NULL;			
		}
		return -1;
	}
	if(s->head == NULL) //追加的缓冲区中不存在未发送的数据
	{
		int n = write(s->fd,request->buffer,request->size);
		if(n == -1)
		{
			switch(errno)
			{
				case EINTR:
				case EAGAIN:
					n = 0;
					break;
				default:
					fprintf(ERR_FILE, "socket_server_send: write to %d (fd=%d) error.",id,s->fd);			
					close_fd(ss,s,result);
					if(request->buffer != NULL)  //error,free memory
					{
						free(request->buffer);
						request->buffer = NULL;
					}
					return -1;
			}
		}
		if(n == request->size) //send success
		{
			if(request->buffer != NULL)
			{
				printf("request->buffer was free\n");
				free(request->buffer);
				request->buffer = NULL;
			}		
			return 0;
		}
		if(n < request->size)
		{
			//append_remaindata(s,request,n);  以后再加上
			//
			epoll_write(ss->epoll_fd,s->fd,s,true);
		}		
	}
	else
	{
		//append_remaindata(s,request,0);
	}
	return 0;
}

