#ifndef ROUNTER_H
#define ROUNTER_H

#include "logic.h"

#define MAX_PLAYER 32*1024  // 等于MAX_SOCKET

struct socket_router
{
	int net_router_fd;
	int logic_router_fd_array[LOGIC_PROCESS_NUM]; 
	int db_router_fd;
	int log_router_fd;
};

#endif
