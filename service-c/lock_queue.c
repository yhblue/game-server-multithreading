#include "lock_queue.h"
#include "err.h"

#include <malloc.h>
#include <stdbool.h>

queue* queue_creat() 
{  
    queue* q = (queue*)malloc(sizeof(queue));  
    if (q == NULL) 
    {  
       fprintf(ERR_FILE,"queue_creat:malloc err\n");
       return NULL;  
    }  
    q->head = NULL;  
    q->tail = NULL;  

    SPIN_INIT(&(q->lock));
    return q;  
}  

q_node* set_qnode(void* buf,char msg_type,char proto_type,int uid,int len,q_node* next)
{
    q_node* qnode = (q_node*)malloc(sizeof(q_node));
    if(qnode == NULL)
    {
        fprintf(ERR_FILE,"set_qnode:qnode malloc failed\n"); 
        return NULL;
    }
    qnode->msg_type = msg_type;
    qnode->proto_type = proto_type;
    qnode->uid = uid;
    qnode->len = len;
    qnode->buffer = buf;
    qnode->next = next; 
    
    return qnode;
}

int queue_push(queue* q,q_node* qnode)
{
    if(qnode == NULL)
    {
        fprintf(ERR_FILE,"queue_push:qnode is null\n"); 
        return -1;
    }

    SPIN_LOCK(&(q->lock));
    if (q->head == NULL) 
    {  
        q->head = qnode;  
    }  
    if (q->tail == NULL) 
    {  
        q->tail = qnode;  
    }  
    else 
    {  
        q->tail->next = qnode;  
        q->tail = qnode;  
    }  
    SPIN_UNLOCK(&(q->lock));    

    return 0;  
}  
  
static bool queue_is_empty(queue* q)
{  
    bool ret = (q->head == NULL);
    return ret;  
}  
  
void* queue_pop(queue* q)
{
	if(queue_is_empty(q) == true)
		return NULL;

    SPIN_LOCK(&(q->lock));
	q_node* qtmp = q->head;
	if(q->head == q->tail)
	{
		q->tail = NULL;
	}
	q->head = q->head->next; 
    SPIN_UNLOCK(&(q->lock)); 
	
	return qtmp; //调用的一方需要释放掉qtmp和qtmp->buffer的内存
}


void queue_destory(queue* q)
{
	while(!queue_is_empty(q))
	{
	   q_node* qtmp = queue_pop(q);
	   if(qtmp != NULL)
	   {
			if(qtmp->buffer != NULL)
			{
				free(qtmp->buffer);
				qtmp->buffer = NULL;
			}
			free(qtmp);
			qtmp = NULL;
	   }
	}
	free(q);
	q = NULL;
}
