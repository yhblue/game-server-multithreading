#include <unistd.h>
#include<string.h>
#include<stdio.h>
#include<malloc.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = 0;
    
    int pipefd[2];
    const char* str = "my name is alice,and this is my story,and the end\n";
    pipe(pipefd);
    pid = fork();
    
    if( pid >0 )  //父进程
    {
      printf("this is parent process\n");
      write(pipefd[1],str,strlen(str)+1); //fd[1] write
      wait(NULL);
      printf("child process end\n");
      close(pipefd[1]);
      exit(0);
    }
    else
    {
      int size = strlen(str) + 1;	
      char *buffer = (char*)malloc(size);
      printf("this is child process\n");
      read(pipefd[0],buffer,size);    //fd[0] read
      printf("read:\n%s\n",buffer);
      close(pipefd[0]);
      free(buffer);
      printf("child process is going to end\n");
      exit(0);
    }
    return 0;
}


