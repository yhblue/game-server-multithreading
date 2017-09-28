#ifndef _LOCK_QUEUE_H
#define _LOCK_QUEUE_H

#include "spin_lock.h"

#include <pthread.h>

//#define NULL_PARAMETER   NULL
#define NULL_TYPE		 0
#define NULL_HEAD		 NULL
#define NULL_BUFFER		 NULL

typedef struct node 
{  
	char  type;				 //预留的字段
	void* msg_head;			 //改成void*类型更具有通用性了
    void* buffer;       	 //for data is data_buffer,for other is NULL
    struct node* next;  
}q_node;

  
typedef struct _queue
{  
    q_node* head;   //指向队头节点  
    q_node* tail;   //指向队尾节点  
    spin_lock lock; //锁
}queue;


queue* queue_creat();
q_node* qnode_create(char type,void* msg_head,void* buffer,q_node* next);
int queue_push(queue* q,q_node* qnode);
void queue_destory(queue* q);
void* queue_pop(queue* q);

#endif
