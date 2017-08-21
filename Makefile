.SUFFIXES:.c .o
CC=gcc
SRCS=fifo.c
OBJS=$(SRCS:.c=.o)
TARGET=fifo

$(TARGET):$(OBJS)
	$(CC) -g -Wall -o $@ $(OBJS)
	@echo '--------complete----------'

.c.o:
	$(CC) -g -Wall -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)