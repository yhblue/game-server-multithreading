.SUFFIXES:.c .o
CC=gcc
SRCS=socket_server.c socket_epoll.c main.c net_logic.c lock_queue.c game_logic.c message.pb-c.c 
OBJS=$(SRCS:.c=.o)
EXE=socket

socket:$(OBJS)
	$(CC) -g -Wall -o $@ $(OBJS) -lpthread -lprotobuf-c
	@echo '--------complete----------'

.c.o:
	$(CC) -g -Wall -o $@ -c $<

clean:
	rm -f $(OBJS) $(EXE)
