#ifndef _PROTO_H
#define _PROTO_H

#include "message.pb-c.h"

#include <malloc.h>
#include <stdbool.h>

#define LOG_REQ  	  		'L'
#define LOG_RSP       		'l'

#define HERO_MSG_REQ  		'H'
#define HERO_MSG_RSP  		'h'

#define HEART_REQ     		'R'
#define HEART_RSP     		'r'

#define ENEMY_MSG     		'e'  //ENEMY_MSG这个信息存储的应该是登陆时候发送的,广播时候应该用另一个才对

#define NEW_ENEMY     		'n'  

#define GAME_START_RSP    	's'

#define GAME_START_REQ 		'S'

#define LOGIN_END 			'i'

#define LEAVE_REQ			'V'
#define LEAVE_RSP			'v'

#define MOVE_REQ  			'M'
#define	MOVE_RSP  			'm'

#define	ENEMY_LEAVE 		'y'


typedef struct _HeroMsg      hero_msg;
typedef struct _LoginReq     login_req;
typedef struct _LoginRsp     login_rsp;
typedef struct _EnemyMsg     enemy_msg;
typedef struct _NewEnemy     new_enemy;
typedef struct _StartReq     start_req;
typedef struct _StartRsp     start_rsp;
typedef struct _LoginEnd 	 login_end;
typedef struct _LeaveReq     leave_req;
typedef struct _LeaveRsp 	 leave_rsp;
typedef struct _MoveReq  	 move_req;
typedef struct _MoveRsp  	 move_rsp;


//***********************************************************************************************************//
//login_rsp:
static inline
void login_rsp_init(void* message)
{
	login_rsp__init(message);
}

static inline
size_t login_rsp_get_packed_size(void *message)
{
	return login_rsp__get_packed_size(message);
}

static inline
size_t login_rsp_pack(const void *message,uint8_t* out)
{
	return login_rsp__pack(message,out);
}

static inline
void* login_rsp_unpack(ProtobufCAllocator  *allocator,size_t len,const uint8_t* data)
{
	return login_rsp__unpack(allocator,len,data);
}

//enemy_msg:
static inline
void enemy_msg_init(void* message)
{
	enemy_msg__init(message);
}

static inline
size_t enemy_msg_get_packed_size(const void* message)
{
	return enemy_msg__get_packed_size(message);
}

static inline
size_t enemy_msg_pack(const void* message,uint8_t* out)
{ 
	return enemy_msg__pack(message,out);
}

static inline
void* enemy_msg_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return enemy_msg__unpack(allocator,len,data);
}

//StartReq:
static inline
void start_req_init(void* message)
{
	start_req__init(message);
}

static inline
size_t start_req_get_packed_size(const void* message)
{
	return start_req__get_packed_size(message);
}

static inline
size_t start_req_pack(const void* message,uint8_t* out)
{
	return start_req__pack(message,out);
}

static inline
void* start_req_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return start_req__unpack(allocator,len,data);
}


//start_rsp
static inline
void start_rsp_init(void* message)
{
	start_rsp__init(message);
}

static inline
size_t start_rsp_get_packed_size(const void* message)
{
	return start_rsp__get_packed_size(message);
}

static inline
size_t start_rsp_pack(const void* message,uint8_t* out)
{
	return start_rsp__pack(message,out);
}

static inline
void start_rsp_free_unpacked(void* message,ProtobufCAllocator *allocator)
{
	start_rsp__free_unpacked(message,allocator);
}


//new_enemy
static inline
void new_enemy_init(void* message)
{
	new_enemy__init(message);
}

static inline
size_t new_enemy_get_packed_size(const void* message)
{
	return new_enemy__get_packed_size(message);
}

static inline
size_t new_enemy_pack(const void* message,uint8_t* out)
{
	return new_enemy__pack(message,out);
}

static inline
void* new_enemy_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return new_enemy__unpack(allocator,len,data);
}

static inline
void new_enemy_free_unpacked(void* message,ProtobufCAllocator* allocator)
{
	new_enemy__free_unpacked(message,allocator);
}

static inline
void login_end_init(void* message)
{
	login_end__init(message);
}

static inline
size_t login_end_get_packed_size(const void* message)
{
	return login_end__get_packed_size(message);
}

static inline
size_t login_end_pack(const void* message,uint8_t* out)
{
	return login_end__pack(message,out);
}

static inline
void* login_end_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return login_end__unpack(allocator,len,data);
}

static inline
void login_end_free_unpacked(void* message,ProtobufCAllocator* allocator)
{
	login_end__free_unpacked(message,allocator);
}

static inline
void leave_rsp_init(void* message)
{
	leave_req__init(message);
}

static inline
size_t leave_req_get_packed_size(const void* message)
{
	return leave_req__get_packed_size(message);
}

static inline
void* leave_req_pack(const void* message,uint8_t* out)
{
	return leave_req__pack(message,out);
}

static inline
void* leave_req_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return leave_req__unpack(allocator,len,data);
}


static inline
void leave_req_free_unpacked(void* message,ProtobufCAllocator* allocator)
{
	leave_req__free_unpacked(message,allocator);
}

static inline
void move_rsp_init(void* message)
{
	move_rsp__init(message);
}

static inline
size_t move_rsp_get_packed_size(const void* message)
{
	return move_rsp__get_packed_size(message);
}

static inline
size_t move_rsp_pack(const void* message,uint8_t* out)
{
	return move_rsp__pack(message,out);
}

static inline
void* move_rsp_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return move_rsp__unpack(allocator,len,data);
}

static inline
void move_rsp_free_unpacked(void* message,ProtobufCAllocator* allocator)
{
	move_rsp__free_unpacked(message,allocator);
}

//move request
static inline
void move_req_init(void* message)
{
	move_req__init(message);
}

static inline
size_t move_req_get_packed_size(const void* message)
{
	return move_req__get_packed_size(message);
}

static inline
size_t move_req_pack(const void* message,uint8_t* out)
{
	return move_req__pack(message,out);
}

static inline
void* move_req_unpack(ProtobufCAllocator* allocator,size_t len,const uint8_t* data)
{
	return move_req__unpack(allocator,len,data);
}

static inline
void move_req_free_unpacked(void* message,ProtobufCAllocator* allocator)
{
	move_req__free_unpacked(message,allocator);
}


/****************************************************************************************/
static login_rsp* login_rsp_creat(bool success,int x,int y,int enemy_num,int uid)
{
	login_rsp* rsp = (login_rsp*)malloc(sizeof(login_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}
	login_rsp_init(rsp);
	rsp->success = success;
	rsp->point_x = x;
	rsp->point_y = y;
	rsp->enemy_num = enemy_num; 
	rsp->uid = uid;  //这个字段要加上

	return rsp;
}

static start_rsp* start_rsp_creat(bool start)
{
	start_rsp* rsp = (start_rsp*)malloc(sizeof(start_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}
	start_rsp_init(rsp);
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
	enemy_msg_init(rsp);
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
	new_enemy_init(rsp);
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
	login_end_init(rsp);
	rsp->success = end;

	return rsp;
}

static leave_rsp* leave_rsp_creat(bool leave)
{
	leave_rsp* rsp = (leave_rsp*)malloc(sizeof(leave_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}
	leave_rsp_init(rsp);
	rsp->leave = leave;	
}

static move_rsp* move_rsp_creat(bool success,int uid,int pos_x,int pos_y)
{
	move_rsp* rsp = (move_rsp*)malloc(sizeof(move_rsp));
	if(rsp == NULL)
	{
		return NULL;
	}	

	rsp->success = success;
	rsp->uid = uid;
	rsp->pos_x = pos_x;
	rsp->pos_y = pos_y;
}

#endif
