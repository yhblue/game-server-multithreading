.SUFFIXES:.c .o
CC=gcc
SRCS=socket_server.c socket_epoll.c epoll_test.c
OBJS=$(SRCS:.c=.o)
EXE=socket

socket:$(OBJS)
	$(CC) -g -Wall -o $@ $(OBJS)
	@echo '--------complete----------'

.c.o:
	$(CC) -g -Wall -o $@ -c $<

clean:
	rm -f $(OBJS) $(EXE)
