.SUFFIXES:.c .o
CC=gcc
SRCS=fifo_write.c
OBJS=$(SRCS:.c=.o)
TARGET=fifo_write

$(TARGET):$(OBJS)
	$(CC) -g -Wall -o $@ $(OBJS)
	@echo '--------complete----------'

.c.o:
	$(CC) -g -Wall -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)
