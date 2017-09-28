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

// q_node* set_qnode(void* buf,char msg_type,char proto_type,int uid,int len,q_node* next)
// {
//     q_node* qnode = (q_node*)malloc(sizeof(q_node));
//     if(qnode == NULL)
//     {
//         fprintf(ERR_FILE,"set_qnode:qnode malloc failed\n"); 
//         return NULL;
//     }
//     qnode->msg_type = msg_type;
//     qnode->proto_type = proto_type;
//     qnode->uid = uid;
//     qnode->len = len;
//     qnode->buffer = buf;
//     qnode->next = next; 
    
//     return qnode;
// }



q_node* qnode_create(char type,void* msg_head,void* buffer,q_node* next)
{
    q_node* qnode = (q_node*)malloc(sizeof(q_node));
    if(qnode == NULL)
    {
        fprintf(ERR_FILE,"set_qnode:qnode malloc failed\n"); 
        return NULL;
    }
    qnode->type = type;
    qnode->msg_head = msg_head;
    qnode->buffer = buffer;
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
    void* ret = NULL;
    SPIN_LOCK(&(q->lock));

    q_node* qtmp = q->head;
    if(qtmp) //not empty
    {
        if(q->head == q->tail)
        {
            q->tail = NULL;
        }
        q->head = q->head->next;  
        ret = qtmp;       
    }

    SPIN_UNLOCK(&(q->lock)); 
	
	return ret; 
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
