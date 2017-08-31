#include "game_logic.h"
#include "socket_server.h"
#include "message.pb-c.h"
#include "net_logic.h"
#include "err.h"
#include "port.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//http://www.cnblogs.com/purpleraintear/p/6160733.html



#define GAME_LOG_EVENT_QUE_NULL          1
#define GAME_LOG_EVENT_SOCKET_CLOSE      2

#define STATE_TYPE_INVALID   			 0
#define STATE_TYPE_USING				 1	

#define START_POSITION_X				 0
#define START_POSITION_Y				 0
#define START_SPEED						 1
#define MAX_USER_NAME_LEN				15

typedef struct _player_msg
{
	int uid;
	char name[MAX_USER_NAME_LEN+1];
}player_msg;

typedef struct _position
{
	int point_x;
	int point_y;
}position;

typedef struct _technique
{
	int speed;
}technique;

typedef struct _player
{
	char state;
	player_msg msg;					
	position pos;
	technique tec;
}player;

typedef struct _map_msg
{
	int length;
	int width;
}map_msg;

//id和map_player怎么对应上
typedef struct _game_logic
{
	int sock_2_net_logic;         	 //通信的socket
	queue* que_2_net_logic;		  	 //通信的消息队列
	queue* service_que;				 //本服务的消息队列
	player* map_player[MAX_MAP];  	 //map_player 是一个指向 sizeof(player) * MAX_PLAYER_EACH_MAP 的头指针
									 //相当于一个 player map_player[MAX_MAP][MAX_PLAYER_EACH_MAP]二维数组
	game_router* route;				 //路由表
	char* netlog_addr;				 
	int netlog_port;
	int service_port;
	fd_set select_set;
	bool check_que;
	int serv_port;
	int game_service_id;			 //记录是第几个游戏逻辑处理服务
	map_msg map;					 //地图信息
}game_logic;


static void game_route_table_creat(game_logic* gl)
{
	gl->route->uid_2_playid = (player_id*)malloc(sizeof(player_id) * MAX_SOCKET);
	if(gl->route->uid_2_playid == NULL)
	{
		fprintf(ERR_FILE,"net_logic_creat:route malloc failed\n");
		return -1;    			
	}
	for(int i=0; i<MAX_SOCKET; i++)
	{
		gl->route->uid_2_playid[i].state = STATE_TYPE_INVALID;
		gl->route->uid_2_playid[i].mapid = 0;
		gl->route->uid_2_playid[i].map_playerid = 0;
	}	
	gl->route->player_num = 0;
	for(int i=0; i<MAX_MAP; i++)
	{
		gl->route->map_player_num[i] = 0;
	}
}


game_logic_start* game_logic_start_creat(queue* que_pool,game_router* route,int service_id,char* netlog_addr,int netlog_port,int serv_port，int game_service_id)
{
	game_logic_start* start = (game_logic_start*)malloc(sizeof(game_logic_start));
	start->que_pool = que_pool;
	start->que_2_net_logic = &que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
	start->service_id = service_id;
	start->service_port = serv_port;
	start->netlog_addr = netlog_addr;
	start->netlog_port = netlog_port;
	start->route = route;
	start->game_service_id = game_service_id;
	return start;
}

static queue* distribute_game_service_que(game_logic* gl,game_logic_start* start)
{
	int game_service_id = gl->game_service_id;
	queue* que = NULL;
	switch(game_service_id)
	{
		case GAME_LOGIC_SERVER_FIRST:
			que = &(start->que_pool[QUE_ID_NETLOGIC_2_GAME_FIRST]);
			break;
		case GAME_LOGIC_SERVER_SECOND:
			que = &(start->que_pool[QUE_ID_NETLOGIC_2_GAME_SECOND]);
			break;
		case GAME_LOGIC_SERVER_THIRD:
			que = &(start->que_pool[QUE_ID_NETLOGIC_2_GAME_THIRD]);
			break;
		case GAME_LOGIC_SERVER_FOURTH:
			que = &(start->que_pool[QUE_ID_NETLOGIC_2_GAME_FOURTH]);
			break;
	}
	return que;
}

static game_logic* game_logic_creat(game_logic_start* start)
{
	game_logic* gl = (game_logic*)malloc(sizeof(game_logic));
	int mapid = 0;
	for(mapid=0; mapid<MAX_MAP; mapid++)
	{
		gl->map_player[mapid] = (player*)malloc(sizeof(player) * MAX_PLAYER_EACH_MAP);
		if(gl->map_player[mapid] == NULL)
		{
			fprintf(ERR_FILE,"game_logic_creat:game service = %d gl->map_player malloc error\n",gl->game_service_id);
			return -1;			
		}
		for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)
		{
			gl->map_player[mapid][i].state = STATE_TYPE_INVALID;
		}
	}
	for(int i=0; i<MAX_MAP; i++)
	{
		gl->route.map_player_num[i] = 0;
	}

	gl->route = start->route;
	gl->netlog_addr = start->netlog_addr;
	gl->netlog_port = start->netlog_port;
	gl->service_port = start->service_port;
	gl->game_service_id = start->game_service_id;

	printf("game logic message\n");
	printf("gl->netlog_addr = %s\n",gl->netlog_addr);
	printf("gl->netlog_port = %d\n",gl->netlog_port);
	printf("game_logic_creat:gl->service_port = %d\n",gl->service_port);

	gl->check_que = false;
	gl->service_que = distribute_game_service_que(gl,start);
	game_route_table_creat(gl);
	FD_ZERO(&gl->select_set); 

	return gl;
}

static bool have_event(game_logic* gl)
{
    FD_SET(gl->sock_2_net_logic,&gl->select_set);
    int ret = select(gl->sock_2_net_logic+1,&gl->select_set,NULL,NULL,NULL);    
	if (ret == 1) 
	{
		return true;
	}
	return false;    
}

static int game_route_get_mapid(game_logic* gl,int uid)
{
	if(gl->route.player_num >= MAX_GAME_SERVICE_PLAYER_NUM)
	{
		return -1; //这个游戏处理服务人数已经满了
	}
	for(uint8_t i=0; i<MAX_MAP; i++)
	{
		if(gl->route.map_player_num[i] < MAX_PLAYER_EACH_MAP)
		{
			return i;
		}
	}
	return -1;	  //全部map满了
}

//返回在mapid地图的MAX_PLAYER_EACH_MAP个成员中的其中一个空闲的成员的id
static int game_route_get_playerid_in_map(game_logic* gl,int mapid)
{
	for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)
	{
		if(gl->map_player[mapid][i].state == STATE_TYPE_INVALID)
		{
			gl->map_player[mapid][i].state = STATE_TYPE_USING;
			return i;
		}
	}
	return -1;	//错误
}

static inline 
player_id* game_route_get_playerid(game_logic* gl,int uid)
{
	if(gl->route->uid_2_playid[uid].state == STATE_TYPE_USING) //是正在使用的成员
	{
		return &(gl->route->uid_2_playid[uid]);
	}
	return NULL;
}

static int game_route_append(game_logic* gl,int uid)
{
	if(gl->route->uid_2_playid[uid].state == STATE_TYPE_INVALID) //可用的成员空间
	{
		int map_id = game_route_get_mapid(gl,uid);
		if(map_id == -1)
		{
			fprintf(ERR_FILE,"game_route_append:game service = %d is player number error\n",gl->game_service_id);
			return -1;
		}
		else
		{
			int  id_in_map = game_route_get_playerid_in_map(map_id);
			if(id_in_map == -1) //error
			{
				fprintf(ERR_FILE,"game_route_append:game service = %d id_in_map error\n",gl->game_service_id);
				return -1;				
			}
			else	//设置路由成员的所有参数
			{
				gl->route->uid_2_playid[uid].state = STATE_TYPE_USING;
				gl->route->uid_2_playid[uid].mapid = map_id;
				gl->route->uid_2_playid[uid].map_playerid = id_in_map;
				gl->route->map_player_num[map_id] ++; //这个map中人数自增
				gl->route->player_num ++;			  //游戏逻辑服人数自增
				return 0;
			}
		}
	}
	return -1;
}

static void game_route_del(game_logic* gl,int uid)
{
	if(gl->route->uid_2_playid[uid].state == STATE_TYPE_USING)
	{
		gl->route->uid_2_playid[uid].state = STATE_TYPE_INVALID;		 //清空路由表对应的这个成员
		int map_id = gl->route->uid_2_playid[uid].mapid;
		int map_playerid = gl->route->uid_2_playid[uid].map_playerid;
		gl->route->map_player_num[map_id] --; 							 
		gl->route->player_num --;			  							 

		gl->map_player[mapid][map_playerid].state = STATE_TYPE_INVALID;  //清空这个成员,设置未被使用
	}
}

typedef struct node 
{  
	char msg_type;			 //'D' 'S' 'C'
    char proto_type;         //for client data ,is serilia type,for inform is 
    int uid;  	        	 //socket uid
    int len;	    		 //for data is buffer length,for other is 0
    void* buffer;       	 //for data is data_buffer,for other is NULL
    struct node* next;  
}q_node;

typedef struct _player
{
	char state;
	player_msg msg;					
	position pos;
	technique tec;
}player;

typedef struct _player_id
{
	char state;						//记录这个成员空间的状态
	uint8_t mapid;     		    	//记录这个用户在哪一个地图上
	uint8_t map_playerid;  			//记录这个用户在mapid上的这个地图的玩家的id 0-9
}player_id;


static void user_msg_load(player* user,void* data_buf,int uid)
{
	login_req* req = (login_req*)data_buf;
	user->msg.uid = uid;
	if(strlen(req->name) <= MAX_USER_NAME_LEN)
	{
		strcpy(user->msg.name,req->name);
	}
	else //客户端有误
	{
		strncpy(user->msg.name,req->name,MAX_USER_NAME_LEN);
	}
	user->pos.point_x = START_POSITION_X;
	user->pos.point_y = START_POSITION_Y;	

	user->tec.speed = START_SPEED;

	free(req->name);
	free(req);
}

//需要把这个地图中的玩家信息发送回请求的玩家
void dispose_login_request(game_logic* gl,player* user,player_id* user_id,void* data_buf,int uid)
{
	user_msg_load(user,data_buf,uid); //信息加载->得到此时的地图内信息->发送给这个玩家
	int player_num = gl->map_player[user_id->mapid];
	for(int i=0; i<player_num; i++)
	{
		
	}
}

static int dispose_game_logic(game_logic* gl,q_node* qnode)
{
	player_id* user_id = game_route_get_playerid(gl,qnode->uid);
	if(user_id != NULL) //success
	{
		player* user = &(gl->map_player[user_id->mapid][user_id->map_playerid]); //得到了最终的存储在游戏逻辑服的这个玩家数据
		switch(qnode->proto_type)
		{
			case LOG_REQ: //登陆请求

				break;
			case:
				break;
			case:
				break;
		}
	}
	return -1;
}

//SUCCESS命令	-创建路由表中玩家成员
//CLOSE         -删除成员
//DATA		    -找到该玩家信息，对信息进行更新
static int dispose_queue_event(game_logic* gl)
{
	printf("game_logic port = %d dispose queue message\n",gl->service_port);
	q_node* qnode = queue_pop(gl->service_que);
	if(qnode == NULL) //队列无数据
	{
		return -1;
	}
	else
	{
		int type = qnode->msg_type;
		switch(type):
		{
			case TYPE_DATA:	    //玩家数据

				break;
			case TYPE_CLOSE:    //玩家关闭
				game_route_del(gl,qnode->uid);
				break;
			case TYPE_SUCCESS:  //新玩家登陆
				game_route_append(gl,qnode->uid);
				break;
		} 
	}
	if(qnode != NULL)
		free(qnode);

	return 0;
}

static int dispose_socket_event(game_logic* gl)
{
	char buf[128];
	int n = read(gl->sock_2_net_logic,buf,sizeof(buf));
	if(n < 0)
	{
		switch(errno)
		{ 
			case EINTR:
			case EAGAIN:
				fprintf(ERR_FILE,"dispose_socket_event: errno\n");
				return 0;    	
			default:
				close(gl->sock_2_net_logic);
				fprintf(ERR_FILE,"dispose_socket_event: game_logic read err,close socket\n");
				return -1;    		
		}
	}
	if(n == 0) //net logic close socket
	{
		close(gl->sock_2_net_logic);
		fprintf(ERR_FILE,"dispose_socket_event: net_logic service close socket\n");
		return -1;
	}	
	return 0;
}

static int game_logic_event(game_logic* gl)
{
	int ret = 0;
	for( ; ; )
	{
		if(gl->check_que == true)
		{
			ret = dispose_queue_event(gl);
			if(ret == -1) //queue is null
			{
				gl->check_que = false;
				return GAME_LOG_EVENT_QUE_NULL;
			}
			else
			{
				continue;
			}
		}
		if(have_event(gl) == true)
		{
			gl->check_que = true;
			printf("game:select change,port = %d\n",gl->service_port);
			if(dispose_socket_event(gl) == -1)
			{
				fprintf(ERR_FILE,"game_logic_event: dispose_socket_event error\n");
				return GAME_LOG_EVENT_SOCKET_CLOSE;				
			}
		}
	}
}


static int connect_netlogic_service(game_logic* gl)
{
    struct sockaddr_in netlog_service_addr;
    struct sockaddr_in game_service_addr;
    
    //creat socket
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if( sockfd == -1 )
    {
       fprintf(ERR_FILE,"connect_netlogic_service:socket creat failed\n");
       return -1;
    }

    game_service_addr.sin_family = AF_INET;
    game_service_addr.sin_port = htons(gl->service_port);			//8002-8005
    game_service_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd,(struct sockaddr*)(&game_service_addr),sizeof(game_service_addr)) == -1)
    {
       fprintf(ERR_FILE,"connect_netlogic_service:socket bind failed\n");
       return -1;    	
    }

    //set service address
    memset(&netlog_service_addr,0,sizeof(netlog_service_addr));
    netlog_service_addr.sin_family = AF_INET;
    netlog_service_addr.sin_port = htons(PORT_NETLOG_LISTENING);
    netlog_service_addr.sin_addr.s_addr = inet_addr(gl->netlog_addr);

    for( ;; )
    {
	    if((connect(sockfd,(struct sockaddr*)(&netlog_service_addr),sizeof(struct sockaddr))) == -1)
	    {
	        fprintf(ERR_FILE,"game:connect_netlogic_service:service disconnect\n");
	        sleep(1);
	    }
	    else
	    {
		    //connect sussess
		    gl->sock_2_net_logic = sockfd;
		    printf("game:game_logic service connect to net_logic service service success!\n");
		    return 0;    	
	    }	
    }
    return 0;	
}


void* game_logic_service_loop(void* arg)
{
	game_logic_start* start = (game_logic_start*)arg;
	game_logic* gl = game_logic_creat(start);

	if(connect_netlogic_service(gl) == -1)
	{
		fprintf(ERR_FILE,"game_logic_service_loop:connect_netlogic_service disconnect\n");
		return NULL;
	}

	int type = 0;
	for( ; ; )
	{
		type = game_logic_event(gl);
		switch(type)
		{
			case GAME_LOG_EVENT_QUE_NULL:
				printf("game_logic:port = %d,queue null\n",gl->service_port);
				break;

			case GAME_LOG_EVENT_SOCKET_CLOSE:
				printf("game_logic:port = %d,socket close\n",gl->serv_port);
				break;
		}
	}

}



