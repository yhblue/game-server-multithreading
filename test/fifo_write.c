#include <fcntl.h>          
#include <sys/stat.h>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
int main()
{
    const char *fifo_name = "/home/python/myfile/fifo";
    int pipe_fd = -1;
    int data_fd = -1;
    int res = 0;
    const int open_mode = O_WRONLY|O_NONBLOCK;
    char buffer[PIPE_BUF+1];
    if(access(fifo_name,F_OK)==-1)
    {
        res = mkfifo(fifo_name,0777);
        if(res!=0)
        {
            fprintf(stderr,"could not create fifo\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("process %d opening fifo O_WRONLY\n",getpid());
    pipe_fd = open(fifo_name,O_RDONLY|O_NONBLOCK);
    pipe_fd = open(fifo_name,open_mode);

    data_fd = open("socket_server.c",O_RDONLY);
    printf("process %d result %d\n",getpid(),pipe_fd);
    printf("process %d result %d\n",getpid(),data_fd);
    if(pipe_fd!=-1)
    {
        int bytes_read = 0;
        printf("command?");
        bytes_read = read(data_fd,buffer,PIPE_BUF);
        while(bytes_read>0)
        {
            res = write(pipe_fd,buffer,bytes_read);
            if(res==-1)
            {
                fprintf(stderr,"write error\n");
                exit(EXIT_FAILURE);
            }
            bytes_read = read(data_fd,buffer,PIPE_BUF);
            buffer[bytes_read]='\0';
        }
        close(pipe_fd);
        close(data_fd);
    }
    else
    {
        exit(EXIT_FAILURE);
    }
    printf("process %d finished.\n",getpid());
    exit(EXIT_SUCCESS);
}
