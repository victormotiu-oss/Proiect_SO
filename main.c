#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define ROLE_MANAGER 1
#define ROLE_INSPECTOR 2

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

int g_role = 0;
char g_user[32] = "";

int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[256];
    strncpy(temp, input, sizeof(temp));
    
    char *token = strtok(temp, ":");
    if (!token) return 0;
    strcpy(field, token);
    
    token = strtok(NULL, ":");
    if (!token) return 0;
    strcpy(op, token);
    
    token = strtok(NULL, "");
    if (!token) return 0;
    strcpy(value, token);
    
    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "==") == 0) return r->severity == val;
    }
    if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
    }
    return 0; 
}

int check_access(const char *filepath, int req_manager, int req_inspector) {
    struct stat st;
    if (stat(filepath, &st) < 0) return 1; 
    
    if (g_role == ROLE_MANAGER && !(st.st_mode & req_manager)) {
        printf("Eroare: Managerul nu are permisiuni (lipseste bitul %o).\n", req_manager);
        return 0;
    }
    if (g_role == ROLE_INSPECTOR && !(st.st_mode & req_inspector)) {
        printf("Eroare: Inspectorul nu are permisiuni (lipseste bitul %o).\n", req_inspector);
        return 0;
    }
    return 1;
}

void log_action(const char *district, const char *action) {
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", district);
    
    struct stat st;
    if (stat(path, &st) == 0 && g_role == ROLE_INSPECTOR && !(st.st_mode & S_IWGRP)) {
        return;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return;

    char entry[256];
    snprintf(entry, sizeof(entry), "[%ld] ROLE:%d USER:%s ACTION:%s\n", time(NULL), g_role, g_user, action);
    write(fd, entry, strlen(entry));
    close(fd);
}

void init_district(const char *district) {
    mkdir(district, 0750);

    char path[256], sym[256];
    
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    int fd = open(path, O_CREAT | O_WRONLY, 0640);
    if (fd >= 0) { write(fd, "threshold=2\n", 12); close(fd); }

    snprintf(path, sizeof(path), "%s/logged_district", district);
    fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) close(fd);

    snprintf(path, sizeof(path), "%s/reports.dat", district);
    snprintf(sym, sizeof(sym), "active_reports-%s", district);
    symlink(path, sym); 
}

void add(const char *district) {
    init_district(district);
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    if (!check_access(path, S_IWUSR, S_IWGRP)) return;

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) {
        perror("Eroare la deschiderea fisierului");
        return;
    }

    Report r;
    r.id = rand() % 10000; 
    r.timestamp = time(NULL);
    strncpy(r.inspector, g_user, 31);

    printf("\n--- Adaugare Raport Nou in %s ---\n", district);
    
    printf("Categorie (ex: road, lighting, flooding): ");
    scanf("%15s", r.category);

    printf("Latitudine (ex: 44.42): ");
    scanf("%f", &r.lat);

    printf("Longitudine (ex: 26.10): ");
    scanf("%f", &r.lon);

    printf("Severitate (1=minor, 2=moderat, 3=critic): ");
    scanf("%d", &r.severity);

    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Descriere: ");
    fgets(r.description, sizeof(r.description), stdin);
    r.description[strcspn(r.description, "\n")] = 0;

    write(fd, &r, sizeof(Report));
    close(fd);
    
    chmod(path, 0664); 
    log_action(district, "ADD");
    
    printf("\n✅ Raport adaugat cu succes! ID-ul generat este: %d\n", r.id);
}

void list(const char *district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    if (!check_access(path, S_IRUSR, S_IRGRP)) return;

    struct stat st;
    if (stat(path, &st) < 0) return;

    char p[] = "---------";
    if (st.st_mode & S_IRUSR) p[0] = 'r'; if (st.st_mode & S_IWUSR) p[1] = 'w'; if (st.st_mode & S_IXUSR) p[2] = 'x';
    if (st.st_mode & S_IRGRP) p[3] = 'r'; if (st.st_mode & S_IWGRP) p[4] = 'w'; if (st.st_mode & S_IXGRP) p[5] = 'x';
    if (st.st_mode & S_IROTH) p[6] = 'r'; if (st.st_mode & S_IWOTH) p[7] = 'w'; if (st.st_mode & S_IXOTH) p[8] = 'x';

    printf("Fisier: %s | Permisiuni: %s | Dimensiune: %ld\n", path, p, st.st_size);

    int fd = open(path, O_RDONLY);
    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        printf("ID: %d | Inspector: %s | Categorie: %s\n", r.id, r.inspector, r.category);
    }
    close(fd);
    log_action(district, "LIST");
}

void view(const char *district, int report_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    if (!check_access(path, S_IRUSR, S_IRGRP)) return;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == report_id) {
            printf("--- DETALII RAPORT %d ---\n", r.id);
            printf("Inspector: %s\nCateg: %s\nSeveritate: %d\nDescriere: %s\n", r.inspector, r.category, r.severity, r.description);
            break;
        }
    }
    close(fd);
    log_action(district, "VIEW");
}

void remove_report(const char *district, int report_id) {
    if (g_role != ROLE_MANAGER) {
        printf("Eroare: Doar managerul poate sterge rapoarte.\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    int fd = open(path, O_RDWR);
    if (fd < 0) return;

    Report r;
    off_t found_pos = -1;
    
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == report_id) {
            found_pos = lseek(fd, 0, SEEK_CUR) - sizeof(Report);
            break;
        }
    }

    if (found_pos != -1) {
        off_t read_pos = found_pos + sizeof(Report);
        off_t write_pos = found_pos;

        while (lseek(fd, read_pos, SEEK_SET) >= 0 && read(fd, &r, sizeof(Report)) > 0) {
            read_pos = lseek(fd, 0, SEEK_CUR);
            lseek(fd, write_pos, SEEK_SET);
            write(fd, &r, sizeof(Report));
            write_pos += sizeof(Report);
        }
        
        ftruncate(fd, write_pos);
        printf("Raport %d sters.\n", report_id);
        log_action(district, "REMOVE");
    } else {
        printf("Raport inexistent.\n");
    }
    close(fd);
}

void update_threshold(const char *district, int new_val) {
    if (g_role != ROLE_MANAGER) {
        printf("Eroare: Doar managerul poate schimba threshold-ul.\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    
    struct stat st;
    stat(path, &st);
    if ((st.st_mode & 0777) != 0640) {
        printf("Eroare: Permisiunile fisierului cfg au fost alterate. Oprire.\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "threshold=%d\n", new_val);
        write(fd, buf, strlen(buf));
        close(fd);
        printf("Threshold actualizat la %d.\n", new_val);
        log_action(district, "UPDATE_THRESHOLD");
    }
}

void filter(const char *district, const char *condition) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    if (!check_access(path, S_IRUSR, S_IRGRP)) return;

    char field[32], op[4], value[64];
    if (!parse_condition(condition, field, op, value)) return;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    printf("Rezultate filtru (%s):\n", condition);
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (match_condition(&r, field, op, value)) {
            printf("- ID: %d | Categ: %s | Sev: %d\n", r.id, r.category, r.severity);
        }
    }
    close(fd);
    log_action(district, "FILTER");
}

int main(int argc, char *argv[]) {
    // Parsare argumente de bază
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) {
            if (strcmp(argv[i+1], "manager") == 0) g_role = ROLE_MANAGER;
            if (strcmp(argv[i+1], "inspector") == 0) g_role = ROLE_INSPECTOR;
        }
        if (strcmp(argv[i], "--user") == 0) {
            strncpy(g_user, argv[i+1], 31);
        }
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--add") == 0) add(argv[i+1]);
        if (strcmp(argv[i], "--list") == 0) list(argv[i+1]);
        if (strcmp(argv[i], "--view") == 0) view(argv[i+1], atoi(argv[i+2]));
        if (strcmp(argv[i], "--remove_report") == 0) remove_report(argv[i+1], atoi(argv[i+2]));
        if (strcmp(argv[i], "--update_threshold") == 0) update_threshold(argv[i+1], atoi(argv[i+2]));
        if (strcmp(argv[i], "--filter") == 0) filter(argv[i+1], argv[i+2]);
    }

    return 0;
}