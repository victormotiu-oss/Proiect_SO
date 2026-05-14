#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    int fd[2];
    pipe(fd);
    pid_t hub_mon=fork();
    if(hub_mon==0)
    {
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        close(fd[1]);
        execlp("./monitor_reports","./monitor_reports",NULL);
    }
    else
    {
        close(fd[1]);
        char b[128];
        while(1)
        {
            int n=read(fd[0],b,sizeof(b)-1);
            b[n]='\0';
            printf(" %s",b);
            fflush(stdout);
        }
    }
}