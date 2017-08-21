#include "game_logic_io.h"
#include "lock_queue.h"

#define MSG_HEAD_SIZE  6

typedef struct _game_logic_io
{
	queue* io2log_que;
	queue* log2io_que;
}game_logic_io;

typedef struct _pack_head
{
	unsigned char content_len;
//	unsigned char buffer_len;
	char msg_type;    //记录哪种消息类型，'D' 数据 'S' 新用户 'C' 客户端关闭
//	char data_type;   //当是数据时候，记录是哪一种数据，用对应的结构体来读取数据
	int uid;     	  //socket的id(或者说是用户的id)
}pack_head;



static game_logic_io* game_logic_io_creat()
{
	int size = sizeof(game_logic_io);
	game_logic_io* gli = (game_logic_io*)malloc(size);

	queue* que01 = queue_creat();
	if(que == NULL)
	{
		fprintf(ERR_FILE,"socket_server_create:queue creat failed\n");
		return NULL;
	}	
	gli->io2log_que = que01;

	queue* que02 = queue_creat();
	if(que == NULL)
	{
		fprintf(ERR_FILE,"socket_server_create:queue creat failed\n");
		return NULL;
	}
	gli->log2io_que = que02;

	return gli;
}



//先读头部6字节，然后根据头部的content_size分配内存读内容
static int dispose_readmessage(int socket)
{
	pack_head* head = (pack_head*)malloc(MSG_HEAD_SIZE);

	int n = (int)read(socket,head,MSG_HEAD_SIZE);
	assert(n == MSG_HEAD_SIZE);
	if(n <= 0)
		goto _err;

	int len = head->content_len;
	char* buffer = (char*)malloc(len);  
	if(buffer == NULL) 
	{
		fprintf(ERR_FILE,"dispose_readmessage: result->buffer is NULL\n");
		return -1;
	}
//	memset(buffer,0,len);	
	int n = (int)read(socket,buffer,len);
	assert(n == len);
	if(n <= 0)
		goto _err;

	return 0;

_err:	
	if(n < 0)
	{
		free(buffer);
		switch(errno)
		{
			case EINTR:
				fprintf(ERR_FILE,"dispose_readmessage: socket read,EINTR\n");
				break;    	// wait for next time
			case EAGAIN:		
				fprintf(ERR_FILE,"dispose_readmessage: socket read,EAGAIN\n");
				break;
			default:
				close_fd(ss,s,result);
				return SOCKET_ERROR;			
		}
	}
	if(n == 0) //client close,important
	{
		free(head);
		free(buffer);
		close(socket);
		return -1;
	}	
}

static void unpack_user_data(game_logic_io* glt,char* data_pack,int len,int type,int uid)
{
	char proto_type = data_pack[0]; 	//记录用的.proto文件中哪个message来序列化  
	char* seria_data = data_pack + 1;   //data_pack 是网络IO线程中分配的内存，反序列化完之后free掉

	switch(proto_type)
	{
		case LOG_REQ:
			LoginReq* msg = login_req__unpack(NULL,len,seria_data);//函数里面malloc了内存，返回给msg使用
			break;												   //当调用socket发送给游戏逻辑进程之后记得free
																   //如果msg的成员有char*类型的，记得先free这个再free msg
		case HERO_MSG_REQ:
			HeroMsg* msg = hero_msg__unpack(NULL,len,seria_data);
			break;

		case CONNECT_REQ:
			ConnectReq* msg = connect_req__unpack(NULL,len,seria_data);
			break;

		case HEART_REQ:
			HeartBeat* msg = heart_beat__unpack(NULL,len,seria_data);
			break;
	}
	q_node* qnode = set_qnode(msg,proto_type,uid,0,NULL); // len 暂时不用

	queue_push(glt->io2log_que,qnode);
}



// static int dispose_io2log_que_event(game_logic_io* gli)
// {
// 	queue* que = gli->io2log_que;
// 	q_node* qnode = queue_pop(que);
// 	if(qnode == NULL) //队列无数据
// 	{
// 		return -1;
// 	}
// 	else
// 	{
// 		char type = qnode->type;  		// 'D' 'S' 'C'
// 		int uid = qnode->uid;
// 		char* data = qnode->buffer;
// 		int content_len = qnode->len; //内容的长度(即客户端原始发送上来的数据的长度)

// 		switch(type)
// 		{
// 			case TYPE_DATA:     // 'D'  数据包--pack--send
// 				send_data = pack_user_data(data,len,uid,type); 		  //pack
// 				send_data_2_game_logic(nl,send_data,len,uid); //send
// 				break;

// 			case TYPE_CLOSE:    //'C'
// 			case TYPE_SUCCESS:  //'S' 


// 			break;
// 		}		
// 	}
// 	return 0;
// }

static int dispose_log2io_que_event(game_logic_io* gli)
{

}