#ifndef _PROTO_H
#define _PROTO_H

#include "message.pb-c.h"

#define LOG_REQ  	  'L'
#define LOG_RSP       'l'

#define HERO_MSG_REQ  'H'
#define HERO_MSG_RSP  'h'

#define CONNECT_REQ   'C'
#define CONNECT_RSP   'c'

#define HEART_REQ     'R'
#define HEART_RSP     'r'

#define ENEMY_MSG     'e'

#define NEW_ENEMY     'n'

#define GAME_START    's'


typedef struct _HeroMsg 	hero_msg;
typedef struct _HeartBeat 	heart_beat;
typedef struct _LoginReq 	login_req;
typedef struct _ConnectReq 	connect_req;
typedef struct _LoginRsp 	login_rsp;
typedef struct _ConnectRsp 	connect_rsp;
typedef struct _EnemyMsg 	enemy_msg;
typedef struct _NewEnemy 	new_enemy;
typedef struct _GameStart 	game_start;

#endif
