#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h> 

typedef struct {
    int id;
    char inspector[32];
    float lat;
    float lon;
    char category[16];
    int severity;
    time_t timestamp;
    char description[128];
} Report;

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Numar insuficient de argumente!\n");
        return -1;
    }
    
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", argv[1]);
    
    int fd = open(path, O_RDONLY);
    if(fd == -1)
    {
        printf("Eroare la deschiderea fisierului pentru districtul %s!\n", argv[1]);
        return -1;
    }
    
    char inspectors[128][32];
    int scores[128] = {0}; // Inițializat cu 0
    int count = 0;
    Report r;
    
    while(read(fd, &r, sizeof(Report)) > 0)
    {
        int found = 0;
        for(int i = 0; i < count; i++)
        {
            if(strcmp(inspectors[i], r.inspector) == 0)
            {
                scores[i] += r.severity;
                found = 1;
                break;
            }
        }
        if(!found)
        {
            strncpy(inspectors[count], r.inspector, 31);
            inspectors[count][31] = '\0'; // Siguranță
            scores[count] = r.severity;
            count++;
        }
    }
    close(fd);

    // AFIȘAREA REZULTATELOR (Aceste texte vor merge prin pipe către city_hub)
    for(int i = 0; i < count; i++) {
        printf("District [%s] -> Inspector: %s | Workload Score: %d\n", argv[1], inspectors[i], scores[i]);
    }

    return 0;
}