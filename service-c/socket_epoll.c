#include "socket_epoll.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int epoll_init()
{
   int efd = epoll_create(1024);
   return efd;
}

int efd_err(int efd)
{
	return efd == -1;
}

int epoll_release(int efd)
{
   return close(efd);
}

int epoll_add(int efd, int sock, void *ud)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = ud;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ev) == -1) 
	{
		return -1;
	}
	return 0;
}

int epoll_del(int efd,int sock)
{
   if(epoll_ctl(efd, EPOLL_CTL_DEL, sock , NULL) ==-1)
   {
   		return -1;
   }
   return 0;
}

int sepoll_wait(int efd, struct event *e, int max)
{
	int i =0;
    struct epoll_event ev[max];
    int ret_n = epoll_wait(efd,ev,max,-1);  //-1没有句柄发生变化，则一直等待
	for (i=0; i<ret_n; i++)                //epoll返回的事件信息存储到自定义的event数组中
	{
		e[i].s_p = ev[i].data.ptr;
		unsigned flag = ev[i].events;
		e[i].write = (flag & EPOLLOUT)? 1 : 0;
		e[i].read = (flag & EPOLLIN)? 1 : 0;
		e[i].error = (flag & EPOLLERR)? 1 : 0;
   }
    return ret_n;
}

int set_nonblock(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if ( flag == -1 ) 
	{
		return -1;
	}
	if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		return -1;
	}
	return 0;
}



int epoll_write(int efd,int sock,void* ud,bool write)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | (write ? EPOLLOUT : 0);
	ev.data.ptr = ud;
	if(epoll_ctl(efd, EPOLL_CTL_MOD, sock, &ev) == -1)
	{
		return -1;
	}	
	return 0;
}


