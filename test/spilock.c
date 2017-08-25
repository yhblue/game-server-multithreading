#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
//#include <spinlock.h>
//#include <include/linux/spinlock.h>
//能不能实现一个消息队列来实现线程之间的通信？消息队列的成员是

#define ERR_FILE stderr
#define TIME  10000

typedef struct node 
{  
    char type;
    int len;
    char* buffer;
    struct node* next;  
}q_node;  
  
typedef struct _queue
{  
    q_node* head; //指向对头节点  
    q_node* tail; //指向队尾节点  
    int spinlock;//
}queue;

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

    //pthread_mutex_init(&(q->mutex_lock),NULL); 
    spin_lock_init(&(q->spinlock));
    return q;  
}  
  
void queue_push(queue* q,char type,void* buf,int buff_len)
{  
    q_node* qnode = (q_node*)malloc(sizeof(q_node));  
    if (qnode == NULL)
    {  
        fprintf(ERR_FILE,"queue_push:malloc err\n"); 
        return;  
    }  

    qnode->type = type;
    qnode->len = buff_len;
    qnode->buffer = buf;
    qnode->next = NULL;  

   	//pthread_mutex_lock(&(q->mutex_lock)); 
    spin_lock(&(q->spinlock));
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
    //pthread_mutex_unlock(&(q->mutex_lock));
    spin_unlock(&(q->spinlock));
}  

int queue_push(queue* q,q_node* qnode)
{
    if(qnode == NULL)
    {
        fprintf(ERR_FILE,"queue_push:qnode is null\n"); 
        return -1;
    }
    
    qnode->next = NULL; //
    //pthread_mutex_lock(&(q->mutex_lock)); 
    spin_lock(&(q->spinlock));
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
    //pthread_mutex_unlock(&(q->mutex_lock)); 
    spin_unlock(&(q->spinlock));      
}

  
bool queue_is_empty(queue* q)
{  
	//pthread_mutex_lock(&(q->mutex_lock));
    bool ret = (q->head == NULL);
    //pthread_mutex_unlock(&(q->mutex_lock));
    return ret;  
}  
  
q_node* queue_pop(queue* q)
{
	if(queue_is_empty(q) == true)
		return NULL;

    spin_lock(&(q->spinlock))
	q_node* qtmp = q->head;
	if(q->head == q->tail)
	{
		q->tail = NULL;
	}
	q->head = q->head->next; 

	spin_unlock(&(q->spinlock));
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


void* que_read(void* arg)
{
    queue* q = arg;
    printf("start que_read thread \n");
    for(int i=0;i<TIME+100;i++)
    {
        q_node* node = queue_pop(q);
        if(node == NULL)
        {

        }
        else
        {
            printf("%c ",node->type);
            printf("%d ",node->len);
            printf("%s\n",node->buffer);
            free(node->buffer);
            free(node);            
        }
    }
}

int main()
{
    pthread_t pthread_id = 0;
    queue* que = queue_creat();

    pthread_create(&pthread_id,NULL,que_read,que);
    printf("start main thread \n");
    for (int i=0;i<TIME;i++)
    {
        char* str1 = "hello world!";
        char* buf1 = (char*)malloc(strlen(str1)+1);
        strcpy(buf1,str1); //buf
        queue_push(que,'d',buf1,strlen(buf1)); 
        usleep(10);

        char* str2 = "HELLO--WORLD!"; 
        char* buf2 = (char*)malloc(strlen(str2)+1);
        strcpy(buf2,str2); 
        queue_push(que,'D',buf2,strlen(buf2));   
        usleep(10);      
    }
    queue_destory(que);
    pthread_join(pthread_id,NULL);
    return 0;
}