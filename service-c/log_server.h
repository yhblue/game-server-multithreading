#ifndef _LOG_SERVER_H
#define _LOG_SERVER_H

#include "lock_queue.h"
#include "net_logic.h"

#define  LEVEL_DEBUG     0
#define  LEVEL_INFO 	 1
#define  LEVEL_WARN	     2
#define  LEVEL_ERROR     3

#define LOG_TYPE_NWTWORK	0
#define lOG_TYPE_ROUTE		1
#define LOG_TYPE_GAME		2

typedef struct _log_server_start
{
	queue* que_pool;
	int server_id;	
	char* network_log_path;
	char* route_log_path;
	char* game_log_path;	
}log_server_start;

#endif
