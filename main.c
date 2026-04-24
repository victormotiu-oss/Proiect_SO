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

// 1. Structura cerută pentru un raport
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


int parse_condition(const char *input, char *field, char *op, char *value) {
    const char *colon1 = strchr(input, ':');
    if (!colon1) return 0;
    const char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) return 0;

    int field_len = colon1 - input;
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    int op_len = colon2 - (colon1 + 1);
    strncpy(op, colon1 + 1, op_len);
    op[op_len] = '\0';

    strcpy(value, colon2 + 1);
    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<") == 0) return r->severity < val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    }
    return 0;
}

// Comanda 1: INIT
void init_district(const char *district) {
    if (mkdir(district, 0775) == -1 && errno != EEXIST) {
        perror("Eroare creare director");
        return;
    }

    char path[512];

    // Creare reports.dat
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0664);
    if (fd != -1) close(fd);

    // Creare district.cfg
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0640);
    if (fd != -1) {
        write(fd, "role=admin\nlevel=1\n", 19);
        close(fd);
    }

    // Creare symlink
    char link_path[512];
    snprintf(link_path, sizeof(link_path), "%s/reports_link", district);
    symlink("reports.dat", link_path);

    printf("Districtul '%s' a fost initializat.\n", district);
}

// Comanda 2: ADD
void add_report(const char *district, char *inspector, float lat, float lon, char *cat, int sev, char *desc) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) {
        printf("Eroare: Ruleaza 'init' pentru districtul '%s' mai intai.\n", district);
        return;
    }

    Report r;
    struct stat st;
    fstat(fd, &st);
    r.id = (st.st_size / sizeof(Report)) + 1; // Generare ID

    strncpy(r.inspector, inspector, STR_LEN);
    r.latitude = lat;
    r.longitude = lon;
    strncpy(r.category, cat, STR_LEN);
    r.severity = sev;
    r.timestamp = time(NULL);
    strncpy(r.description, desc, DESC_LEN);

    write(fd, &r, sizeof(Report));
    close(fd);
    printf("Raport %d adaugat in %s.\n", r.id, district);
}

// Comanda 3: READ ALL / LIST
void read_all(const char *district) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Nu exista rapoarte in %s.\n", district);
        return;
    }

    Report r;
    printf("\n--- Rapoarte in %s ---\n", district);
    printf("%-3s | %-15s | %-12s | %-3s | %s\n", "ID", "Inspector", "Categorie", "Sev", "Data");
    while (read(fd, &r, sizeof(Report)) > 0) {
        printf("%-3d | %-15s | %-12s | %-3d | %s", r.id, r.inspector, r.category, r.severity, ctime(&r.timestamp));
    }
    close(fd);
}

// Comanda 4: FILTER
void filter_reports(const char *district, const char *condition) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    char field[STR_LEN], op[10], value[STR_LEN];
    if (!parse_condition(condition, field, op, value)) {
        printf("Eroare: Format conditie invalid. Foloseste camp:operator:valoare\n");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) return;

    Report r;
    printf("\n--- Rezultate Filtrare in %s ---\n", district);
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (match_condition(&r, field, op, value)) {
            printf("ID %d | Categ: %s | Sev: %d | Desc: %s\n", r.id, r.category, r.severity, r.description);
        }
    }
    close(fd);
}

// Comanda 5: REMOVE REPORT
void remove_report(const char *district, int id_to_remove) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        printf("Eroare la deschiderea fisierului.\n");
        return;
    }

    struct stat st;
    fstat(fd, &st);
    int total_reports = st.st_size / sizeof(Report);
    Report r;
    int found_index = -1;

    // 1. Cautam indexul raportului
    for (int i = 0; i < total_reports; i++) {
        lseek(fd, i * sizeof(Report), SEEK_SET);
        read(fd, &r, sizeof(Report));
        if (r.id == id_to_remove) {
            found_index = i;
            break;
        }
    }

    // 2. Daca l-am gasit, mutam rapoartele de dupa el cu o pozitie in spate
    if (found_index != -1) {
        for (int i = found_index + 1; i < total_reports; i++) {
            lseek(fd, i * sizeof(Report), SEEK_SET);
            read(fd, &r, sizeof(Report));

            lseek(fd, (i - 1) * sizeof(Report), SEEK_SET);
            write(fd, &r, sizeof(Report));
        }

        // 3. Taiem finalul fisierului
        ftruncate(fd, (total_reports - 1) * sizeof(Report));
        printf("Raportul cu ID %d a fost sters din %s.\n", id_to_remove, district);
    } else {
        printf("Raportul cu ID %d nu a fost gasit.\n", id_to_remove);
    }

    close(fd);
}

// Comanda 6: UPDATE THRESHOLD
void update_threshold(const char *district, int new_threshold) {
    char path[512];
    snprintf(path, sizeof(path), "%s/district.cfg", district);

    // Deschidem fisierul cu O_RDWR (Citire si Scriere)
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        printf("Eroare: Nu s-a putut deschide %s/district.cfg\n", district);
        return;
    }

    char buffer[1024] = {0};
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Eroare la citirea configuratiei");
        close(fd);
        return;
    }

    char new_content[1024] = {0};
    char *pos = strstr(buffer, "threshold=");

    if (pos != NULL) {
        // Daca "threshold=" exista deja, rescriem fisierul inlocuind valoarea veche
        int prefix_len = pos - buffer;
        strncpy(new_content, buffer, prefix_len);

        char *end_of_line = strchr(pos, '\n');
        char temp[64];
        snprintf(temp, sizeof(temp), "threshold=%d\n", new_threshold);
        strcat(new_content, temp);

        if (end_of_line != NULL) {
            strcat(new_content, end_of_line + 1);
        }
    } else {
        // Daca nu exista, copiem ce era inainte si adaugam threshold-ul la final
        strcpy(new_content, buffer);
        if (strlen(new_content) > 0 && new_content[strlen(new_content) - 1] != '\n') {
            strcat(new_content, "\n"); // Ne asiguram ca scriem pe rand nou
        }
        char temp[64];
        snprintf(temp, sizeof(temp), "threshold=%d\n", new_threshold);
        strcat(new_content, temp);
    }

    // Trunchiem fisierul la 0 bytes (il stergem de continutul vechi)
    ftruncate(fd, 0);
    // Mutam cursorul inapoi la inceput
    lseek(fd, 0, SEEK_SET);
    // Scriem noul text in fisier
    write(fd, new_content, strlen(new_content));

    printf("Threshold actualizat cu succes la %d in fisierul %s/district.cfg\n", new_threshold, district);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Comenzi disponibile:\n");
        printf(" %s init <district>\n", argv[0]);
        printf(" %s add <district> <inspector> <lat> <long> <categorie> <gravitate> <descriere>\n", argv[0]);
        printf(" %s list <district>\n", argv[0]);
        printf(" %s filter <district> <conditie>\n", argv[0]);
        printf(" %s remove <district> <id>\n", argv[0]);
        printf(" %s update_threshold <district> <valoare>\n", argv[0]);
        return 1;
    }

    char *command = argv[1];
    char *district = argv[2];

    if (strcmp(command, "init") == 0) {
        init_district(district);
    } else if (strcmp(command, "add") == 0 && argc == 10) {
        add_report(district, argv[3], atof(argv[4]), atof(argv[5]), argv[6], atoi(argv[7]), argv[8]);
    } else if (strcmp(command, "list") == 0) {
        read_all(district);
    } else if (strcmp(command, "filter") == 0 && argc == 4) {
        filter_reports(district, argv[3]);
    } else if (strcmp(command, "remove") == 0 && argc == 4) {
        remove_report(district, atoi(argv[3]));
    } else if (strcmp(command, "update_threshold") == 0 && argc == 4) {
        update_threshold(district, atoi(argv[3]));
    } else {
        printf("Comanda invalida sau numar gresit de argumente.\n");
    }

    return 0;
}
