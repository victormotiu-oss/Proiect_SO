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

    void calculate_score(const char **district,int number_of_districts) 
    {
        for(int i=0;i<number_of_districts;i++)
        {
            // Implementation for calculating score for each district
            int pfd[2];
            if(pipe(pfd)==-1)
            {
                perror("Eroare la pipe!\n");
                exit(1);
            }
            pid_t pid_score=fork();
            if(pid_score<0)
            {
                perror("Eroare la fork!\n");
                close(pfd[0]);
                close(pfd[1]);
            }
            if(pid_score==0)
            {
                close(pfd[0]);
                if(dup2(pfd[1],STDOUT_FILENO)==-1)
                {
                    perror("Eroare la dup2!\n");
                    close(pfd[1]);
                    exit(1);
                }
            
            }

        }
    }
}