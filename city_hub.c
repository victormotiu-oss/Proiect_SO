#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// Funcție pentru pornirea monitorului
void start_monitor() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Eroare la crearea pipe-ului");
        return;
    }

    pid_t hub_mon = fork();
    if (hub_mon < 0) {
        perror("Eroare la fork hub_mon");
        return;
    }

    if (hub_mon == 0) {
        // --- PROCESUL HUB_MON ---
        pid_t monitor_pid = fork();
        
        if (monitor_pid == 0) {
            // --- PROCESUL MONITOR ---
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO); // Redirectăm ieșirea standard în pipe
            close(fd[1]);
            execlp("./monitor_reports", "./monitor_reports", NULL);
            perror("Eroare la executarea monitor_reports");
            exit(1);
        } else {
            // --- HUB_MON CITEȘTE DIN PIPE ---
            close(fd[1]);
            char b[256];
            int n;
            // Această buclă stă pe fundal și printează ce zice monitorul
            while ((n = read(fd[0], b, sizeof(b) - 1)) > 0) {
                b[n] = '\0';
                // Detectăm dacă s-a oprit
                if (strstr(b, "ies")) {
                    printf("\n[HUB] Atentie: Monitorul s-a inchis!\n");
                } else {
                    printf("\n[HUB_MON a primit]: %s", b);
                }
                fflush(stdout);
            }
            close(fd[0]);
            exit(0);
        }
    } else {
        printf("Monitorul a fost pornit in background!\n");
    }
}

// Funcție pentru calcularea scorurilor
void calculate_scores(char *args) {
    if (args == NULL || strlen(args) == 0) {
        printf("Eroare: Trebuie sa specifici cel putin un district!\n");
        return;
    }

    // Împărțim string-ul cu districte (ex: "centru nord")
    char *district = strtok(args, " \n");
    
    printf("\n--- REZULTATE SCORURI ---\n");

    while (district != NULL) {
        int pfd[2];
        if (pipe(pfd) == -1) {
            perror("Eroare la pipe");
            return;
        }

        pid_t pid_score = fork();
        if (pid_score < 0) {
            perror("Eroare la fork");
            return;
        }

        if (pid_score == 0) {
            close(pfd[0]);
            if (dup2(pfd[1], STDOUT_FILENO) == -1) {
                perror("Eroare la dup2");
                exit(1);
            }
            close(pfd[1]);
            
            // Executăm programul extern scorer, pasând numele districtului
            execlp("./scorer", "./scorer", district, NULL);
            perror("Eroare la executarea scorer");
            exit(1);
        } else {
            // --- PROCESUL PĂRINTE (HUB) ---
            close(pfd[1]); // Părintele doar citește
            char buf[512];
            int n;
            
            // Citim output-ul generat de ./scorer
            while ((n = read(pfd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                printf("%s", buf);
            }
            
            close(pfd[0]);
            waitpid(pid_score, NULL, 0); // Așteptăm să se termine scorer-ul curent
        }
        
        district = strtok(NULL, " \n"); // Trecem la următorul district din listă
    }
    printf("---------\n");
}

int main() {
    char input[256];

    printf("=== CITY HUB INTERACTIV ===\n");
    printf("Comenzi: start_monitor, calculate_scores <districte>, exit\n");

    // Bucla interactivă
    while (1) {
        printf("hub> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        input[strcspn(input, "\n")] = 0; // Scoatem tasta Enter
        if (strlen(input) == 0) continue;

        char *cmd = strtok(input, " ");
        char *args = strtok(NULL, ""); // Extragem argumentele (restul rândului)

        if (strcmp(cmd, "start_monitor") == 0) {
            start_monitor();
        } 
        else if (strcmp(cmd, "calculate_scores") == 0) {
            calculate_scores(args);
        } 
        else if (strcmp(cmd, "exit") == 0) {
            printf("Se inchide hub-ul...\n");
            system("killall monitor_reports 2>/dev/null");
            break;
        } 
        else {
            printf("Comanda necunoscuta!\n");
        }
    }

    return 0;
}