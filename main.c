#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define STR_LEN 64
#define DESC_LEN 256

typedef struct {
    int id;
    char inspector[STR_LEN];
    float latitude;
    float longitude;
    char category[STR_LEN];
    int severity;
    time_t timestamp;
    char description[DESC_LEN];
} Report;

void init_district(const char *dir_name) {
    if (mkdir(dir_name, 0775) == -1) {
        if (errno != EEXIST) {
            perror("Eroare la crearea directorului");
            return;
        }
    }

    char path[512];

    snprintf(path, sizeof(path), "%s/reports.dat", dir_name);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0664);
    if (fd != -1) {
        printf("Creat: %s\n", path);
        close(fd);
    }

    // 3. Creare district.cfg (Permisiuni rw-r----- -> 0640)
    snprintf(path, sizeof(path), "%s/district.cfg", dir_name);
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0640);
    if (fd != -1) {
        const char *default_cfg = "role=admin\nlevel=1\n";
        write(fd, default_cfg, strlen(default_cfg));
        printf("Creat: %s\n", path);
        close(fd);
    }

    // 4. Creare Legătură Simbolică (symlink)
    char link_path[512];
    snprintf(link_path, sizeof(link_path), "%s/reports_link", dir_name);
    // Symlink-ul punctează către fișierul din interiorul aceluiași folder
    if (symlink("reports.dat", link_path) == 0) {
        printf("Creat link simbolic: %s -> reports.dat\n", link_path);
    } else if (errno != EEXIST) {
        perror("Eroare la creare symlink");
    }

    printf("Districtul '%s' a fost initializat cu succes.\n", dir_name);
}

void add_report(const char *dir_name, char *inspector, float lat, float lon, char *cat, int sev, char *desc) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", dir_name);
    
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) { 
        printf("Eroare: Districtul nu exista. Ruleaza 'init' intai.\n"); 
        return; 
    }

    Report r;
    struct stat st;
    fstat(fd, &st);
    r.id = (st.st_size / sizeof(Report)) + 1;
    
    strncpy(r.inspector, inspector, STR_LEN);
    r.latitude = lat;
    r.longitude = lon;
    strncpy(r.category, cat, STR_LEN);
    r.severity = sev;
    r.timestamp = time(NULL);
    strncpy(r.description, desc, DESC_LEN);

    write(fd, &r, sizeof(Report));
    close(fd);
    printf("Raport adaugat in %s cu ID: %d\n", dir_name, r.id);
}

void list_reports(const char *dir_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", dir_name);
    
    int fd = open(path, O_RDONLY);
    if (fd == -1) { printf("Nu exista rapoarte in acest district.\n"); return; }

    Report r;
    printf("\n--- Rapoarte District: %s ---\n", dir_name);
    printf("%-3s | %-15s | %-12s | %-3s | %s", "ID", "Inspector", "Categorie", "Sev", "Data");
    while (read(fd, &r, sizeof(Report)) > 0) {
        printf("%-3d | %-15s | %-12s | %-3d | %s", r.id, r.inspector, r.category, r.severity, ctime(&r.timestamp));
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Utilizare:\n");
        printf("  %s init <nume_district>\n", argv[0]);
        printf("  %s add <nume_district> <inspector> <lat> <long> <categorie> <gravitate> <descriere>\n", argv[0]);
        printf("  %s list <nume_district>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        init_district(argv[2]);
    } else if (strcmp(argv[1], "add") == 0 && argc == 9) {
        add_report(argv[2], argv[3], atof(argv[4]), atof(argv[5]), argv[6], atoi(argv[7]), argv[8]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_reports(argv[2]);
    } else {
        printf("Comanda necunoscuta sau argumente insuficiente.\n");
    }

    return 0;
}