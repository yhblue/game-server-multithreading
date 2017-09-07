#ifndef _PROTO_H
#define _PROTO_H

#include "message.pb-c.h"

#include <malloc.h>
#include <stdbool.h>

#define LOG_REQ  	  		'L'
#define LOG_RSP       		'l'

#define HERO_MSG_REQ  		'H'
#define HERO_MSG_RSP  		'h'

#define CONNECT_REQ   		'C'
#define CONNECT_RSP   		'c'

#define HEART_REQ     		'R'
#define HEART_RSP     		'r'

#define ENEMY_MSG     		'e'  //ENEMY_MSG这个信息存储的应该是登陆时候发送的,广播时候应该用另一个才对

#define NEW_ENEMY     		'n'  

#define GAME_START_RSP    	's'

#define GAME_START_REQ 		'S'

#define LOGIN_END 			'i'

typedef struct _HeroMsg      hero_msg;
typedef struct _LoginReq     login_req;
typedef struct _LoginRsp     login_rsp;
typedef struct _EnemyMsg     enemy_msg;
typedef struct _NewEnemy     new_enemy;
typedef struct _StartReq     start_req;
typedef struct _StartRsp     start_rsp;
typedef struct _LoginEnd 	 login_end;


static login_rsp* login_rsp_creat(bool success,int x,int y,int enemy_num,int uid)
{
	login_rsp* rsp = (login_rsp*)malloc(sizeof(login_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}
	rsp->success = success;
	rsp->point_x = x;
	rsp->point_y = y;
	rsp->enemy_num = enemy_num; 
//	rsp->uid = uid;  //这个字段要加上

	return rsp;
}

static start_rsp* start_rsp_creat(bool start)
{
	start_rsp* rsp = (start_rsp*)malloc(sizeof(start_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}
	rsp->start = start;

	return rsp;
}

static enemy_msg* enemy_msg_creat(int uid,int x,int y)
{
	enemy_msg* rsp = (enemy_msg*)malloc(sizeof(enemy_msg));
	if(rsp == NULL)
	{
		return NULL;
	}
	rsp->uid = uid;
	rsp->point_x = x;
	rsp->point_y = y;

	return rsp;	
}

static new_enemy* new_enemy_creat(int uid,int x,int y)
{
	new_enemy* rsp = (new_enemy*)malloc(sizeof(new_enemy));
	if(rsp == NULL)
	{
		return NULL;
	}
	rsp->uid = uid;
	rsp->point_x = x;
	rsp->point_y = y;

	return rsp;
}

static login_end* login_end_creat(bool end)
{
	login_end* rsp = (login_end*)malloc(sizeof(login_end));
	if(rsp == NULL)
	{
		return NULL;
	}
	rsp->success = end;

	return rsp;
}

//***********************************************************************************************************//

static inline
size_t login_rsp_get_packed_size(const login_rsp *message)
{
	return login_rsp__get_packed_size(message);
}

static inline
size_t login_rsp_pack(const login_rsp *message,uint8_t* out)
{
	return login_rsp__pack(message,out);
}

static inline
login_rsp* login_rsp_unpack(ProtobufCAllocator  *allocator,size_t len,const uint8_t* data)
{
	return login_rsp__unpack(allocator,len,data);
}

#endif
