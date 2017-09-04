#ifndef _LOCK_QUEUE_H
#define _LOCK_QUEUE_H

#include "spin_lock.h"

#include <pthread.h>
//网关服务中网络IO处理线程与事务处理线程通信的数据类型约定
//消息队列的双方的通信约定
#define TYPE_DATA     'D'   //普通数据包
#define TYPE_CLOSE    'C'   //客户端关闭
#define TYPE_SUCCESS  'S'   //新客户端完全登陆成功


typedef struct _head
{
	char msg_type;			 //'D' 'S' 'C'
    char proto_type;         //for client data ,is serilia type,for inform is 
    int uid;  	        	 //socket uid
    int len;	    		 //for data is buffer length,for other is 0	
}head;

typedef struct node 
{  
	void* msg_head;			 //改成void*类型更具有通用性了
    void* buffer;       	 //for data is data_buffer,for other is NULL
    struct node* next;  
}q_node;

typedef struct node 
{  
	char msg_type;			 //'D' 'S' 'C'
    char proto_type;         //for client data ,is serilia type,for inform is 
    int uid;  	        	 //socket uid
    int len;	    		 //for data is buffer length,for other is 0
    void* buffer;       	 //for data is data_buffer,for other is NULL
    struct node* next;  
}q_node;
  
typedef struct _queue
{  
    q_node* head; //指向对头节点  
    q_node* tail; //指向队尾节点  
//    pthread_mutex_t mutex_lock;
    spin_lock lock;
}queue;

queue* queue_creat();
q_node* set_qnode(void* buf,char msg_type,char proto_type,int uid,int len,q_node* next);
int queue_push(queue* q,q_node* qnode);
void queue_destory(queue* q);
void* queue_pop(queue* q);

#endif
