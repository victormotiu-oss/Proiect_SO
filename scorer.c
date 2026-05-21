#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
    char inspector[32];
    int severity;
} Report;

int main(int argc, char *argv[])
{
    if(argc<2)
    {
        perror("Numar insuficient de argumente!\n");
        return -1;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", argv[1]);
    int fd=open(path, O_RDONLY);
    if(fd==-1)
    {
        perror("Eroare la deschiderea fisierului!\n");
        return -1;
    }
    char inspectors[128][32];
    int scores[128];
    int count=0;
    Report r;
    while(read(fd,&r,sizeof(Report))>0)
    {
        int found=0;
        for(int i=0;i<count;i++)
        {
            if(strcmp(inspectors[i],r.inspector)==0)
            {
                scores[i]+=r.severity;
                found=1;
                break;
            }
        }
        if(!found)
        {
            strncpy(inspectors[count],r.inspector,31);
            scores[count]=r.severity;
            count++;
        }
    }


}
