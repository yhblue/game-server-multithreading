#ifndef _GAME_LOGIC_H
#define _GAME_LOGIC_H

#define MAX_PLAYER 		  	30
#define LOGIC_PROCESS_NUM 	50

struct close_req
{
	int id;
};

struct send_data_req
{
	int id;               	//4
	int size;            	//4
	char *buffer;	    	//8   max 120
};

struct package_close
{
	uint8_t type;
	uint8_t len;
	uint8_t buffer[6];
	struct close_req close;	
};

struct package_send_data
{
	uint8_t type;
	uint8_t len;
	uint8_t buffer[6];
	struct send_data_req data;
};


#pragma pack(1)
struct player_msg
{
	char name[16];
};

struct position
{
	int src_x;
	int src_y;
	int dest_x;
	int dest_y;	
};

struct technique
{
	int life;
	int gold;
};

struct player
{
	struct player_msg count;
	struct position pos;
};

struct server_player
{
	int player_num;              //这个进程里面的游戏总人数
	int 
	struct player* player_pool;  //每一个逻辑进程维护50个 player 结构体

};

//每个玩家与网络IO进程通信的共享内存基础形式
struct logic2net_data
{
	int id;          //要发送的socket的id
	int write_sz;	 //要发送的字节数
	union
	{
	 	char buffer[128];  		//发送内容,占住128字节 实际上小于128
	 	struct player player;   //只发送账号和位置了，发送账号是为了客户端知道哪个是自己
	}data;
};

struct logic2net_shm
{
	int play_num; 	 //本次buffer中有效的数据成员个数
	struct logic2net_data buffer[MAX_PLAYER]; //一个地图内最多30个玩家
};

struct logic2net_shm_all
{
	struct logic2net_shm shm_array[LOGIC_PROCESS_NUM]; //最终所有的logic进程创建的共享内存
};





#pragma pack()