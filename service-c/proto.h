#ifndef _PROTO_H
#define _PROTO_H

#include "message.pb-c.h"

#define LOG_REQ  	  		'L'
#define LOG_RSP       		'l'

#define HERO_MSG_REQ  		'H'
#define HERO_MSG_RSP  		'h'

#define CONNECT_REQ   		'C'
#define CONNECT_RSP   		'c'

#define HEART_REQ     		'R'
#define HEART_RSP     		'r'

#define ENEMY_MSG     		'e'

#define NEW_ENEMY     		'n'

#define GAME_START_RSP    	's'

#define GAME_START_REQ 		'S'




typedef struct _HeroMsg 	hero_msg;
typedef struct _HeartBeat 	heart_beat;
typedef struct _LoginReq 	login_req;
typedef struct _ConnectReq 	connect_req;
typedef struct _LoginRsp 	login_rsp;
typedef struct _ConnectRsp 	connect_rsp;
typedef struct _EnemyMsg 	enemy_msg;
typedef struct _NewEnemy 	new_enemy;
typedef struct _GameStart 	game_start;

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
	rsp->uid = uid;

	return rsp;
}

static game_start* game_start_creat(bool start)
{
	game_start* rsp = (game_start*)malloc(sizeof(game_start));
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




#endif
