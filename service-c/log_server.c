#include "log_server.h"
#include "lock_queue.h"

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#define MAX_LOG_CONTENT_SIZE	256    //log一次最多256个字节，避免一次log内容需要多行显示

typedef struct _log_server
{
	char* network_log_path;
	char* route_log_path;
	char* game_log_path;	
	queue* server_queue;
	int server_id;
	bool que_check;
	char* log_content;//256字节
}log_server;


//log服务应该要有那几个字段
//1.level级别
//2.类型标识
//3.日期-时间
//4.内容:


static log_server* log_server_create(net_logic_start* start)
{
	log_server* ls = (log_server*)malloc(sizeof(log_server));
	if(ls == NULL)
	{
		return NULL;
	}
	ls->log_content = (char*)malloc(MAX_LOG_CONTENT_SIZE);
	if(ls->log_content == NULL)
	{
		return NULL;
	}
	memset(ls->log_content,0,MAX_LOG_CONTENT_SIZE);

	ls->network_log_path = start->network_log_path;
	ls->route_log_path = start->route_log_path;
	ls->game_log_path = start->game_log_path;
	ls->server_queue =  start->que_pool[QUE_ID_LOG_SERVER];
	ls->que_check = true;

	return ls;
}



log_server_start* log_server_start_create(queue* que_pool,configure* conf,int server_id)
{
	log_server_start* start = (log_server_start*)malloc(sizeof(log_server_start));
	if(start == NULL)
	{
		return NULL;
	} 
	start->network_log_path = conf->network_log_path;
	start->route_log_path = conf->route_log_path;
	start->game_log_path = conf->game_log_path;
	start->que_pool = que_pool;
	start->server_id = server_id;
	return start;
}

static int dispose_queue_event(log_server* ls)
{
	q_node* qnode = queue_pop(ls->server_queue);
	if(qnode == NULL) //队列无数据
	{
		printf("queue null\n");
		return -1;
	}
	else 
	{
		//根据消息的头部信息，把数据写进相应的文件中
		if(qnode != NULL)
			free(qnode);		
	}
	return 0;	
}

static void sleep_wait(int sec)
{
	sleep(sec);
}

static int log_server_event(log_server* ls)
{
	int ret = 0;
	for(; ;)
	{
		if(ls->que_check)
		{
			ret = dispose_queue_event(ls);
			if(ret == -1)
			{
				ls->que_check = false;
				return -1;
			}
			else
			{
				continue;
			}
		}
		else
		{
			sleep_wait(1);
			ls->que_check = true;
		}
	}
} 


void get_current_time()
{
	time_t time_seconds = time(0);  
    struct tm now_time;  
    localtime_r(&time_seconds, &now_time);  
  	sprintf(ls->log_content,"%d-%d-%d %d:%d:%d", now_time.tm_year + 1900, now_time.tm_mon + 1,  
          now_time.tm_mday, now_time.tm_hour, now_time.tm_min, now_time.tm_sec);  
//  	printf("%s\n",ls->log_content);
}

void log(int level,int type,char* format,...)
{

}

void log(char* format,...)
{
	printf(format,...);
}

void* log_server_loop(void* arg)
{
	log_server_start* start = (log_server_start*)arg;
	if(start == NULL)
		return NULL;

	log_server* ls = log_server_create(start);
	if(ls == NULL)
		return NULL;

	for(; ;)
	{

	}
}



