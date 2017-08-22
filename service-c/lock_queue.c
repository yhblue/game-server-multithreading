#include "lock_queue.h"
#include "err.h"

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

    pthread_mutex_init(&(q->mutex_lock),NULL); 
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
    
    pthread_mutex_lock(&(q->mutex_lock)); 
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
    pthread_mutex_unlock(&(q->mutex_lock));       
}  
  
bool queue_is_empty(queue* q)
{  
    pthread_mutex_lock(&(q->mutex_lock));
    bool ret = (q->head == NULL);
    pthread_mutex_unlock(&(q->mutex_lock));
    return ret;  
}  
  
void* queue_pop(queue* q)
{
	if(queue_is_empty(q) == true)
		return NULL;

	pthread_mutex_lock(&(q->mutex_lock));
	q_node* qtmp = q->head;
	if(q->head == q->tail)
	{
		q->tail = NULL;
	}
	q->head = q->head->next; 
	pthread_mutex_unlock(&(q->mutex_lock));
	
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





// void queue_push(queue* q,char type,void* buf,int len)
// {  
//     q_node* qnode = (q_node*)malloc(sizeof(q_node));  
//     if (qnode == NULL)
//     {  
//         fprintf(ERR_FILE,"queue_push:malloc err\n"); 
//         return;  
//     }  

//     qnode->type = type;
//     qnode->len = len;
//     qnode->buffer = buf;
//     qnode->next = NULL;  

//      pthread_mutex_lock(&(q->mutex_lock)); 
//     if (q->head == NULL) 
//     {  
//         q->head = qnode;  
//     }  
//     if (q->tail == NULL) 
//     {  
//         q->tail = qnode;  
//     }  
//     else 
//     {  
//         q->tail->next = qnode;  
//         q->tail = qnode;  
//     }  
//     pthread_mutex_unlock(&(q->mutex_lock));
// }