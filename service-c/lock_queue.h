#ifndef _LOCK_QUEUE_H
#define _LOCK_QUEUE_H

#include <pthread.h>
//网关服务中网络IO处理线程与事务处理线程通信的数据类型约定
//消息队列的双方的通信约定
#define TYPE_DATA     'D'   //普通数据包
#define TYPE_CLOSE    'C'   //客户端关闭
#define TYPE_SUCCESS  'S'   //新客户端完全登陆成功


#define SIZEOF_QNODE  32   //消息队列的节点32字节

typedef struct node 
{  
		char msg_type;
    char proto_type;         //for client data ,is serilia type,for inform is 
    int uid;  	        	//socket uid
    int len;	    		//for data is buffer length,for other is 0
    void* buffer;       	//for data is data_buffer,for other is NULL
    struct node* next;  
}q_node;
  
typedef struct _queue
{  
    q_node* head; //指向对头节点  
    q_node* tail; //指向队尾节点  
    pthread_mutex_t mutex_lock;
}queue;

queue* queue_creat();
q_node* set_qnode(void* buf,char msg_type,char proto_type,int uid,int len,q_node* next);
int queue_push(queue* q,q_node* qnode);
void queue_destory(queue* q);
void* queue_pop(queue* q);

#endif
