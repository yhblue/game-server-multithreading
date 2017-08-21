#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

int main()
{
    const char *fifo_name = "/home/python/myfile/fifo";
    int pipe_fd = -1;
    int data_fd = -1;
    int res = 0;
    int open_mode = O_RDONLY;
    char buffer[PIPE_BUF+1];
    int bytes_read = 0;
    int bytes_write = 0;
    memset(buffer,'\0',sizeof(buffer));

    printf("process %d opening FIFO O_RDONLY\n",getpid());
    pipe_fd = open(fifo_name,open_mode);
    data_fd = open("dataformfifo.txt",O_WRONLY|O_CREAT,0644);
    printf("process %d result %d\n",getpid(),pipe_fd);
    if(pipe_fd!=-1)
    {
        do{
            res = read(pipe_fd,buffer,PIPE_BUF);
            bytes_write = write(data_fd,buffer,res);
            bytes_read +=res;
        }while(res>0);
        close(pipe_fd);
        close(data_fd);
    }
    else{
        exit(EXIT_FAILURE);
    }
    printf("process %d finished,%d bytes read\n",getpid(),bytes_read);
    exit(EXIT_SUCCESS);
}