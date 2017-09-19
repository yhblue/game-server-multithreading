#include "game_logic.h"
#include "socket_server.h"
#include "message.pb-c.h"
#include "configure.h"
#include "err.h"
#include "port.h"
#include "proto.h"

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


#define GAME_LOG_EVENT_QUE_NULL          1
#define GAME_LOG_EVENT_SOCKET_CLOSE      2

#define STATE_TYPE_INVALID   			 0
#define STATE_TYPE_USING				 1	

#define PLAYER_UID_NULL			        -1
#define	MAP_PLAYER_STATE_unchanged		 0
#define	MAP_PLAYER_STATE_CHANGEED		 1		

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

	char* service_route_addr;
	int service_route_port;
	int service_port;

	fd_set select_set;
	bool check_que;
	int serv_port;
	int game_service_id;			 //记录是第几个游戏逻辑处理服务
	map_msg map;					 //地图信息
}game_logic;

player_id* playerid_list_creat(void)
{
	player_id* uid_2_playid = (player_id*)malloc(sizeof(player_id) * MAX_SOCKET);
	if(uid_2_playid == NULL)
	{
		fprintf(ERR_FILE,"playerid_list_creat:uid_2_playid malloc failed\n");
		return NULL;		
	}
	for(int i=0; i<MAX_SOCKET; i++)
	{
		uid_2_playid[i].state = STATE_TYPE_INVALID;
		uid_2_playid[i].mapid = 0;
		uid_2_playid[i].map_playerid = 0;
	}
	return uid_2_playid;
}


static int game_route_table_creat(game_logic* gl,game_logic_start* start)
{
	gl->route = (game_router*)malloc(sizeof(game_router));

	if(gl->route == NULL)
	{
		fprintf(ERR_FILE,"game_route_table_creat:route malloc failed\n");
		return -1;   		
	}
	gl->route->uid_2_playid = start->uid_2_playid;

	gl->route->player_num = 0;
	for(int i=0; i<MAX_MAP; i++)
	{
		gl->route->map_player_num[i] = 0;
	}
	int mapid = 0;
	for(mapid=0; mapid<MAX_MAP; mapid++)
	{
		gl->route->map_player_socket_uid[mapid] = (int*)malloc(sizeof(int) * MAX_PLAYER_EACH_MAP);
		if(gl->route->map_player_socket_uid[mapid] == NULL)
		{
			fprintf(ERR_FILE,"game_logic_creat:game service = %d gl->map_player_uid malloc error\n",gl->game_service_id);
			return -1;				
		}
		for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)
		{
			gl->route->map_player_socket_uid[mapid][i] = PLAYER_UID_NULL;
		}
	}
	for(mapid=0; mapid<MAX_MAP;mapid++)
	{
		gl->route->map_uid_list[mapid] = (int*)malloc(sizeof(int) * MAX_PLAYER_EACH_MAP);
		if(gl->route->map_uid_list[mapid] == NULL)
		{
			fprintf(ERR_FILE,"game_logic_creat:game service = %d gl->map_player_uid malloc error\n",gl->game_service_id);
			return -1;				
		}
		for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)
		{
			gl->route->map_uid_list[mapid][i] = 0;
		}
	}
	return 0;
}

game_logic_start* game_logic_start_creat(queue* que_pool,configure* conf,player_id* uid_2_playid,int service_id)
{
	game_logic_start* start = (game_logic_start*)malloc(sizeof(game_logic_start));
	start->que_pool = que_pool;
	start->que_2_net_logic = &que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
	switch(service_id)
	{
		case SERVICE_ID_GAME_FIRST:
			start->game_service_id = GAME_LOGIC_SERVER_FIRST;
			start->service_port = PORT_GAME_LOGIC_SERVICE_FIRST;
			break;

		case SERVICE_ID_GAME_SECOND:
			start->game_service_id = GAME_LOGIC_SERVER_SECOND;
			start->service_port = PORT_GAME_LOGIC_SERVICE_SECOND;
			break;		
		
		case SERVICE_ID_GAME_THIRD:
			start->game_service_id = GAME_LOGIC_SERVER_THIRD;
			start->service_port = PORT_GAME_LOGIC_SERVICE_THIRD;
			break;	

		case SERVICE_ID_GAME_FOURTH:
			start->game_service_id = GAME_LOGIC_SERVER_FOURTH;
			start->service_port = PORT_GAME_LOGIC_SERVICE_FOURTH;
			break;	
	}
	start->service_route_addr = conf->service_address;
	start->service_route_port = conf->service_route_port;	//路由的端口

	start->uid_2_playid = uid_2_playid;
	start->service_id = service_id; 				
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
			return NULL;			
		}
		for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)
		{
			gl->map_player[mapid][i].state = STATE_TYPE_INVALID;
		}
	}

	gl->service_route_addr = start->service_route_addr;
	gl->service_route_port = start->service_route_port;

	gl->service_port = start->service_port;
	gl->game_service_id = start->game_service_id;

	printf("game logic message\n");
	printf("gl->service_route_port = %d\n",gl->service_route_port);
	printf("game_logic_creat:gl->service_port = %d\n",gl->service_port);

	gl->check_que = false;
	gl->service_que = distribute_game_service_que(gl,start);
	gl->que_2_net_logic = &start->que_pool[QUE_ID_GAMELOGIC_2_NETLOGIC];
	game_route_table_creat(gl,start);
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
	if(gl->route->player_num >= MAX_GAME_SERVICE_PLAYER_NUM)
	{
		return -1; //这个游戏处理服务人数已经满了
	}
	for(uint8_t i=0; i<MAX_MAP; i++)
	{
		if(gl->route->map_player_num[i] < MAX_PLAYER_EACH_MAP)
		{
			return i;
		}
	}
	return -1;	  //全部map满了
}

//返回在mapid地图的MAX_PLAYER_EACH_MAP个成员中的其中一个空闲的成员的id
static int game_route_get_playerid_in_map(game_logic* gl,int mapid)
{
	int i = 0; 
	for(i=0; i<MAX_PLAYER_EACH_MAP; i++)
	{
		if(gl->map_player[mapid][i].state == STATE_TYPE_INVALID)
		{
			gl->map_player[mapid][i].state = STATE_TYPE_USING;  //标记已使用
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
		int map_id = game_route_get_mapid(gl,uid); //分配一个可用的地图的地址
		if(map_id == -1)
		{
			fprintf(ERR_FILE,"game_route_append:game service = %d is player number error\n",gl->game_service_id);
			return -1;
		}
		else
		{
			int  id_in_map = game_route_get_playerid_in_map(gl,map_id);
			if(id_in_map == -1) //error
			{
				fprintf(ERR_FILE,"game_route_append:game service = %d id_in_map error\n",gl->game_service_id);
				return -1;				
			}
			else	//设置路由成员的所有参数
			{
				if(gl->route->map_player_socket_uid[map_id][id_in_map] == PLAYER_UID_NULL) //socket uid的这个成员空
				{
					gl->route->uid_2_playid[uid].state = STATE_TYPE_USING;     		//标记为已经被使用
					gl->route->uid_2_playid[uid].mapid = map_id;
					gl->route->uid_2_playid[uid].map_playerid = id_in_map;	   
					gl->route->map_player_socket_uid[map_id][id_in_map] = uid; 		//记录这个uid，用于广播
				    gl->route->player_num ++;			                       		//游戏逻辑服人数自增		
					// 当游戏开始时候才更新地图中玩家数目,但是让 gl->route->player_num 先增加，标记这个游戏逻辑服人数已经增加
					printf("~~~~~~game: uid = %d map_id = %d,map_playerid = %d gl->route->map_player_num=%d~~~~~~~~~\n",uid,map_id,id_in_map,gl->route->map_player_num[map_id]);

					return 0;			
				}
				else
				{
					fprintf(ERR_FILE,"game_route_append:game service = %d map_player_socket_uid error\n",gl->game_service_id);
					return -1;	
				}
			}
		}
	}
	return -1;
}

static int game_route_del(game_logic* gl,int uid)
{
	if(gl->route->uid_2_playid[uid].state == STATE_TYPE_USING)
	{
		gl->route->uid_2_playid[uid].state = STATE_TYPE_INVALID;		 //清空路由表对应的这个成员
		int map_id = gl->route->uid_2_playid[uid].mapid;
		int map_playerid = gl->route->uid_2_playid[uid].map_playerid;
		gl->route->map_player_num[map_id] --; 							 
		gl->route->player_num --;			  							 
		
		if(gl->route->map_player_socket_uid[map_id][map_playerid] == uid)
		{
			gl->route->map_player_socket_uid[map_id][map_playerid] = PLAYER_UID_NULL;
		}
		else
		{
			fprintf(ERR_FILE,"game_route_del:game service = %d map_player_socket_uid error\n",gl->game_service_id);
			return -1;				
		}

		gl->map_player[map_id][map_playerid].state = STATE_TYPE_INVALID;  //清空这个成员,设置未被使用

		return 0;
	}
	return -1;
}

static void user_msg_load(player* user,void* data_buf,int uid)
{
	login_req* req = (login_req*)data_buf;
	user->msg.uid = uid;
	if(strlen(req->name) <= MAX_USER_NAME_LEN)
	{
		strcpy(user->msg.name,req->name);
		printf("game:user msg load! name = %s\n",req->name);
	}
	else //客户端有误
	{
		strncpy(user->msg.name,req->name,MAX_USER_NAME_LEN);
	}

	if(user == NULL)
	{
		printf("user is null\n");
		return;
	}
	user->pos.point_x = START_POSITION_X;
	user->pos.point_y = START_POSITION_Y;	

	user->tec.speed = START_SPEED;

	free(req->name);
	free(req);
}

//得到要广播uid这个玩家的广播uid列表
//把n-1个整数从src复制到des,
static void uid_list_cpy(int* des,int* src,int n)
{
	for(int index=0; index<n; index++)
	{
		des[index] = src[index];
	}
}

//登陆时候发送不同的类型的数据
static int login_msg_send(game_logic* gl,int hero_uid,int enemy_uid,char msg_type)
{
	broadcast* broadcast_msg = (broadcast*)malloc(sizeof(broadcast));
	if(broadcast_msg == NULL)
	{
		fprintf(ERR_FILE,"send_login_rsp: broadcast_msg malloc error\n");
		return -1;
	}

	broadcast_msg->list.broadcast_player_num = 1; 			//只有一个玩家
	broadcast_msg->list.uid_list[0] = hero_uid;		  		//玩家的uid

	player_id* user_id = &(gl->route->uid_2_playid[hero_uid]);
	player* user = &(gl->map_player[user_id->mapid][user_id->map_playerid]); //得到playerid
	int player_num = gl->route->map_player_num[user_id->mapid];	             //根据playerid 得到最终存储信息的player

	void * rsp = NULL;
	switch(msg_type)
	{
		case LOG_RSP:
			rsp = login_rsp_creat(true,user->pos.point_x,user->pos.point_y,player_num,hero_uid);
			break;			

		case ENEMY_MSG:									
			rsp = enemy_msg_creat(enemy_uid,user->pos.point_x,user->pos.point_y);
			break;				

		case LOGIN_END:
			rsp = login_end_creat(true); 
			break;

		case GAME_START_RSP:
			rsp = start_rsp_creat(true);
			break;
	}
	if(rsp == NULL)
	{
		fprintf(ERR_FILE,"send_login_rsp: rsp malloc error\n");
		return -1;		
	}
	broadcast_msg->data.proto_type = msg_type;
	printf("\n\n&&&&&&& game: send type = %c to netlogic \n\n",msg_type);
	broadcast_msg->data.buffer = rsp;

	q_node* qnode = qnode_create(&broadcast_msg->list,&broadcast_msg->data,NULL);
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"send_login_rsp: qnode malloc error\n");
		return -1;			
	}
	queue_push(gl->que_2_net_logic,qnode);    

	send_msg2_service(gl->sock_2_net_logic);	

	return 0;
}

void map_uid_list_rebuild(game_logic* gl,int uid)
{
	player_id* user_id = game_route_get_playerid(gl,uid);
	if(user_id == NULL)
	{
		return;
	}
	int mapid = user_id->mapid;
	printf("--------rebuild: mapid = %d----------\n",mapid);
	int index = 0;

	for(int i=0; i<MAX_PLAYER_EACH_MAP; i++)  //10
	{
		int user_id = gl->route->map_player_socket_uid[mapid][i]; 
		printf("+++++++++game rebuild:user_id = %d ++++++\n",user_id);
		if((user_id != PLAYER_UID_NULL) && (user_id != uid))    //那么不为空的放在 map_uid_list[] 最前面
		{
			printf("*******game rebuild:user_id = %d ********\n",user_id);
			gl->route->map_uid_list[mapid][index++] = user_id; //把有效的uid都放在最前面
		}
	}
	printf("rebuild list index = %d,player = %d\n ",index,gl->route->map_player_num[mapid]);
//	assert(index == (gl->route->map_player_num[mapid]-1));
}


static void map_uid_list_append(game_logic* gl,int mapid,int uid)
{
	int index = gl->route->map_player_num[mapid] - 1; //总人数-1 = index 
	printf("\n\n\n!!!------list append index = %d --------\n",index);
	gl->route->map_uid_list[mapid][index] = uid; 
}


static int broadcast_player_msg(game_logic* gl,player* user,player_id* user_id,char broadcast_msg_type)
{
	broadcast* broadcast_msg = (broadcast*)malloc(sizeof(broadcast));
	if(broadcast_msg == NULL)
	{
		fprintf(ERR_FILE,"send_login_rsp: broadcast_msg malloc error\n");
		return -1;
	}
	int player_num = gl->route->map_player_num[user_id->mapid];
	broadcast_msg->list.broadcast_player_num = player_num;				//得到这个地图的人数
	int *uid_list = gl->route->map_uid_list[user_id->mapid];								
	uid_list_cpy(broadcast_msg->list.uid_list,uid_list,player_num);     //复制广播列表

	void *rsp = NULL;
	switch(broadcast_msg_type) //广播的数据的 pack 类型
	{
		case NEW_ENEMY:
			rsp = new_enemy_creat(user->msg.uid,user->pos.point_x,user->pos.point_y);
			break;

		case ENEMY_MSG:
			rsp = enemy_msg_creat(user->msg.uid,user->pos.point_x,user->pos.point_y);
			break;
	}
	if(rsp == NULL)
	{
		fprintf(ERR_FILE,"broadcast_player_msg: rsp malloc error\n");
		return -1;	
	}
	broadcast_msg->data.proto_type = broadcast_msg_type;
	broadcast_msg->data.buffer = rsp;

	q_node* qnode = qnode_create(&broadcast_msg->list,&broadcast_msg->data,NULL);
	if(qnode == NULL)
	{
		fprintf(ERR_FILE,"send_login_rsp: qnode malloc error\n");
		return -1;			
	}
	queue_push(gl->que_2_net_logic,qnode);    

	send_msg2_service(gl->sock_2_net_logic);	

	return 0;
}


//需要把这个地图中的玩家信息发送回请求的玩家
//信息加载->发送登陆确认回复->得到此时的地图内信息->发送给这个玩家->广播给其他玩家有新玩家登陆->发送开始游戏信息
static void dispose_login_request(game_logic* gl,player_id* user_id,void* data,int uid)
{
	printf("****game:user_id->mapid = %d user_id->map_playerid =%d *****\n",user_id->mapid,user_id->map_playerid);
	player* user = &(gl->map_player[user_id->mapid][user_id->map_playerid]);
	user_msg_load(user,data,uid); 

	login_msg_send(gl,uid,0,LOG_RSP);								//发送登陆确认回复

	int map_player_num = gl->route->map_player_num[user_id->mapid]; //这个地图中的玩家数目,此时并不包括自己
	
	for(int i=0; i<map_player_num; i++) 							//得到地图内其他玩家信息-发送给这个登录的玩家
	{
		int enemy_uid = gl->route->map_uid_list[user_id->mapid][i];	//得到同一个地图中其他敌军的uid
		login_msg_send(gl,uid,enemy_uid,ENEMY_MSG);					//发送所有其他玩家信息
	}

	login_msg_send(gl,uid,0,LOGIN_END);								//登陆流程结束
}

static int dispose_start_request(game_logic* gl,player_id* user_id,void* data,int uid)
{							
	start_req* req = (start_req*)data;
	if(req->start == true)
	{
		printf("^^^^^^^game: client require start is true^^^^^^^\n");
		player* user = &(gl->map_player[user_id->mapid][user_id->map_playerid]);		

		broadcast_player_msg(gl,user,user_id,NEW_ENEMY);					 //广播给其他玩家有新玩家登陆

		gl->route->map_player_num[user_id->mapid] ++;						 //地图内玩家自增

		map_uid_list_append(gl,user_id->mapid,uid);							 //追加广播列表
		login_msg_send(gl,uid,0,GAME_START_RSP);							 //发送开始游戏	
	}

	free(data);
	return 0;
}

static int dispose_game_logic(game_logic* gl,q_node* qnode)
{
	msg_head* head = (msg_head*)qnode->msg_head;
	int uid = head->uid;
	void* data = qnode->buffer;
	player_id* user_id = game_route_get_playerid(gl,uid);	//根据uid得到playerid
	printf("-----game: client:uid = %d,gameid = %d,user_id->mapid = %d,user_id->map_playerid = %d----\n",uid,gl->game_service_id,user_id->mapid,user_id->map_playerid);
	if(user_id != NULL)
	{
		switch(head->proto_type)
		{
			case LOG_REQ: 		 //登陆请求
				printf("#######game:recieve que login request#######\n");
				dispose_login_request(gl,user_id,data,uid);
				break;

			case GAME_START_REQ: //开始游戏请求
				//广播新玩家进入地图 -> 地图人数+1 -> 广播列表追加 -> 回应开始游戏
				printf("#######game:recieve que game start request#######\n");
				dispose_start_request(gl,user_id,data,uid); //this function have error
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
		msg_head* head = (msg_head*)qnode->msg_head;
		char type = head->msg_type;
		switch(type)
		{
			case TYPE_DATA:	    //玩家数据
				printf("\n\n<<<<<<<<game:client data>>>>>>>\n");
				dispose_game_logic(gl,qnode);
				break;

			case TYPE_CLOSE:    //玩家关闭
				printf("\n\n<<<<<<<game service: client close uid = %d>>>>>>>\n",head->uid);
				map_uid_list_rebuild(gl,head->uid);		//广播列表重建 
				game_route_del(gl,head->uid); 			//在路由表中删除该成员 
				break;

			case TYPE_SUCCESS:  //新玩家登陆
				printf("\n\n<<<<<<<<game service: new client uid = %d>>>>>>>>\n",head->uid);
				game_route_append(gl,head->uid);
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

	int optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		perror("game service:setsockopt bind\n");
		return -1; 
	}
	    
    if(bind(sockfd,(struct sockaddr*)(&game_service_addr),sizeof(game_service_addr)) == -1)
    {
       fprintf(ERR_FILE,"connect_netlogic_service:socket bind failed\n");
       return -1;    	
    }

    //set service address
    memset(&netlog_service_addr,0,sizeof(netlog_service_addr));
    netlog_service_addr.sin_family = AF_INET;
    netlog_service_addr.sin_port = htons(gl->service_route_port);
    netlog_service_addr.sin_addr.s_addr = inet_addr(gl->service_route_addr);

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
		    printf("game:game_logic port = %d service connect to net_logic service service success!\n",gl->service_port);
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
	return NULL;
}

