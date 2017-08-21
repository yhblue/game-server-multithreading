//主要完成消息的转发，有一个消息过来了我要知道把这个消息转发到哪里
//逻辑处理进程有数据过来，要发送给数据给网络进程，我要告诉网络进程某个逻辑处理进程有数据过来了，要求你去取来发送
//当网络进程处理完这个数据后，返回一个消息给路由进程，让他告诉逻辑进程已经发送完数据了。
//问题：
//1)那怎么知道一个玩家数据过来，能够判断他现在在哪个逻辑进程？
//那就是玩家登陆时候，网络io进程给通过net_router_fd这个socket告诉router有新用户到来了，那么我写在这个router进程里面维护一个结构体，
//存储每一个logic进程的人数，然后写一个函数，这个函数的功能是，返回这个用户应该到的那个logic进程的id号。
//
//网络进程的接收功能是：这个socket有变化了，epoll返回，读数据(假如读的数据是已经经过粘包处理过了的)，那读数据的缓冲区采用
//的是开辟的共享内存？读到数据之后，告诉路由器进程哪些socket有新的数据，路由进程根据这个socket所在的logic进程得到要向哪
//一个逻辑进程发送数据。

//问题：
//2)那我网络进程读数据，读到共享内存后，通知去处理，如果logic进程还没读走上一次放进去共享内存的数据呢？
//这时候这个socket又返回数据了，那么我该用这个共享内存去存储读的数据吗？
//那就再做一个判断了，如果这个共享内存有新的数据,置1，取走后，置0，epoll读取前先判断一下id对应的共享内存的标志位是不是0，
//是就存，否则，跳过，等待下一次读取。
//
//既然一个逻辑进程只开辟一个线程去处理50个场景，那么能不能给每一个场景开辟一个共享内存？
//

#include "err.h"
#include "socket_epoll.h"

#include <router.h>
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


#define ROUTER_MAX_CONNECT 64
#define PROCESS_ALL 60
#define ROUTER_MAX_EVENT   64 //一次最多返回64个事件

#define PROCESS_NET    1
#define PROCESS_LOGIC  2
#define PROCESS_DB	   3
#define PROCESS_LOG    4
#define PROCESS_ROUTER 5

struct online_player
{
	uint8_t online_num_array[LOGIC_PROCESS_NUM]; //存储每一个logic进程的在线人数，新增人数+1，下线-1
}

struct process_epoll_ud
{
	int process_id;
	int socket_fd;
	int type;
};

struct router
{
	int epoll_fd;
	int event_index;
	int event_num;
	uint8_t socket_logic_pool[MAX_PLAYER]; 		//存储id对应的logic进程的id
	int socket_id2fd[LOGIC_PROCESS_NUM];   		//存储router与其他进程通信的socket信息，根据数组下标(id)得到socket_fd;
	struct online_player player_num;       		//存储每一个逻辑进程的在线人数
	struct process_epoll_ud* process_ud_pool;
	struct event* event_pool;
};


//返回这个玩家应该去哪一个逻辑进程
int get_logic_process(struct router *rt)
{
	int i = 0;
	for(i; i<LOGIC_PROCESS_NUM; i++)
	{
		if(player_num[i] < MAX_PLAYER)
		{
			return i;
		}
	}
	return -1;	//所以区人数都满了
}



//根据传递进来的socket的id值找到这个id所在的是哪一个logic进程
//router通知这个logic进程有数据要处理了，让他去读取数据
int socket_id2logic_fd(int id)
{
	
}




struct router* router_creat()
{
	struct router *rt = (struct router*)malloc(sizeof(struct router));
	if(rt == NULL)
		return -1;
	int efd = epoll_init();
	if(efd_err(efd))
	{
		fprintf(ERR_FILE,"socket_server_create:epoll create failed\n");
		return NULL;
	}
	rt->epoll_fd = efd;

	int size = sizeof(process_epoll_ud)*PROCESS_ALL;
	rt->process_ud_pool = (struct process_epoll_ud*)malloc(size); 
	if(rt->process_ud_pool == NULL)
	{
		fprintf(ERR_FILE,"router_creat:process_ud_pool malloc failed\n");
		epoll_release(efd);
		return NULL;		
	}
	memset(rt->process_ud_pool,0,size);

	size = sizeof(struct event)*ROUTER_MAX_EVENT;
	rt->event_pool = (struct event*)malloc(size);
	if(rt->event_pool == NULL)
	{
		fprintf(ERR_FILE,"router_creat:event_pool malloc failed\n");
		free(rt->process_ud_pool);
		epoll_release(efd);
		return NULL;	
	}
	memset(rt->event_pool,0,size);
}


int router_listen_creat(struct router *rt,const char* host,int port,int max_connect)
{
    int  listen_fd;
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

void router_listen_start(struct router *rt)
{
	int listen_fd = router_listen_creat();
	if(listen_fd == -1)
	{
		fprintf(ERR_FILE,"router_listen_start:router_listen_creat failed\n"); 
		return;
	}
	rt->socket_fd = listen_fd;
	rt->process_ud_pool[0].type = PROCESS_ROUTER;
	if(epoll_add(rt->epoll_fd,listen_fd,&(rt->process_ud_pool[0])) == -1)
	{
		fprintf(ERR_FILE,"router_listen_start:epoll_add failed\n"); 
		return;
	}
}

int router_epoll_event(struct router *rt)
{
	for( ; ; )
	{
		if(rt->event_index = rt->event_num)
		{
			rt->event_num = sepoll_wait(rt->epoll_fd,rt->event_pool,ROUTER_MAX_EVENT);
			if(rt->event_num <= 0) //error
			{
				fprintf(ERR_FILE,"router_epoll_event:sepoll_wait return error event_n\n");
				rt->event_num = 0;
				return -1;
			}	
			rt->event_index = 0;			
		}
		struct event* eve = &(rt->event_pool[rt->event_index++]);
		struct process_epoll_ud* process_ud = eve->s_p; 
		if(process_ud == NULL)
		{
			fprintf(ERR_FILE,"router_epoll_event:a NULL process_ud\n");
			return -1;				
		}
		switch(process_ud->type)
		{
			case PROCESS_NET:
				break;
			case PROCESS_LOGIC:
				break;
			case PROCESS_DB:
				break;
			case PROCESS_LOG:
				break;
			case PROCESS_ROUTER:
				break;
		}
	} 
	
}

void router_process_loop()
{
	struct router *rt = router_creat();

}


