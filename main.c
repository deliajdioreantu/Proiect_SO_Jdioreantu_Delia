#include <stdio.h>
#include <sys/stat.h>
#include<unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include<time.h>

#define NAME 50
#define CAT 30
#define DESCR 100
#define PATH_SIZE 256
#define LINK_SIZE 200
#define LOG_SIZE 100
typedef struct {
    float latitude;
    float longitude;
}GPS;
typedef struct {
    int id;
    char inspectorName[NAME];
    GPS gps;
    char category[CAT];
    int severity;
    time_t timestamp;
    char description[DESCR];
}REPORT;

void create(const char *district) {
    if (mkdir(district, 0750) == -1) {
        if (errno != EEXIST) {
            perror("Mkdir error\n");
            return;
        }
    }// Setare explicita pentru siguranta
    chmod(district, 0750);

    char path[PATH_SIZE];
    int f;

    //reports.dat
    snprintf(path,sizeof(path),"%s/reports.dat", district);
    f=open(path, O_CREAT | O_RDWR, 0664);
    if (f == -1) {
        perror("Open error reports.dat");
        return;
    }
    chmod(path, 0664);
    close(f);

    //distrct.cfg
    snprintf(path, sizeof(path),"%s/district.cfg",district);
    f=open(path, O_CREAT | O_RDWR, 0640);
    if (f!=-1) {
        if (lseek(f,0,SEEK_END)==0){//scriem pragul daca fisierul e nou
            char buf[]="Threshold=1\n";
            write(f,buf,strlen(buf));
        }
        chmod(path, 0640);  // ignora umask, garanteaza permisiunile exacte
        close(f);
    }

    //logged_district
    snprintf(path, sizeof(path),"%s/logged_district",district);
    f=open(path, O_CREAT | O_RDWR, 0644);
    chmod(path, 0644);
    close(f);

    //symlink
    char link_name[LINK_SIZE];
    snprintf(link_name,sizeof(link_name),"active_reports-%s",district); //numele linkului
    snprintf(path,sizeof(path),"%s/reports.dat",district); //"tinta", catre cine duce linkul
    symlink(path, link_name);
}

void add_report(const char *district,const char *user) {
    REPORT r;
    char path[PATH_SIZE];
    int f;

    create(district);
    printf("X: "); scanf("%f", &r.gps.latitude);
    printf("Y: "); scanf("%f", &r.gps.longitude);

    printf("Category (road/lighting/flooding/other): ");
    scanf("%s",r.category);

    printf("Severity level (1/2/3): ");
    scanf("%d",&r.severity);

    getchar(); // Consumă newline-ul rămas în buffer pentru ca fgets să funcționeze [cite: 132]
    printf("Description: ");
    fgets(r.description, DESCR, stdin);
    r.description[strcspn(r.description, "\n")] = 0;

    snprintf(path, sizeof(path), "%s/reports.dat", district);
    f=open(path,O_RDWR,0664);
    struct stat st;
    stat(path, &st); //citim informatii despre fisier
    r.id=st.st_size /sizeof(REPORT) +1;

    strncpy(r.inspectorName, user, NAME-1);
    r.timestamp=time(NULL);

    lseek(f,0,SEEK_END);
    if (write(f, &r, sizeof(REPORT)) == -1) {
        perror("Error saving report!\n");
    } else {
        printf("Succes! Raportul a fost salvat cu ID: %d\n", r.id);
    }
}

void add_logged_district(const char *district,const char *user, const char *role,const char *action){
    char path[PATH_SIZE];
    char log[LOG_SIZE];
    int f;

    snprintf(path,sizeof(path),"%s/logged_district", district);
    f=open(path,O_WRONLY| O_APPEND,0644);

    time_t now=time(NULL);
    snprintf(log,sizeof(log),"%s\t%ld\t%s\t%s\n",action,(long)now,role,user);
    write(f,log,sizeof(log));
    close(f);
}

void list(const char *district) {
    REPORT r;
    char path[PATH_SIZE];
    int f;
    snprintf(path,sizeof(path),"%s/reports.dat",district);
    f=open(path,O_RDONLY);
    if (f==-1) {
        perror("Error opening reports.dat file!\n");
        return;
    }

    while (read(f,&r,sizeof(REPORT)) >0) {
        printf("ID: %d\n", r.id);
        printf("Inspector: %s\n", r.inspectorName);
        printf("GPS: %.2f, %.2f\n", r.gps.latitude, r.gps.longitude);
        printf("Category: %s\n", r.category);
        printf("Severity: %d\n", r.severity);
        printf("Timestamp: %s", ctime(&r.timestamp));
        printf("Description: %s\n", r.description);
        printf("-------------------\n");
    }

    struct stat st;
    stat(path,&st);
    if (stat(path, &st) == -1) {
        perror("Eroare la citirea atributelor fisierului");
        return;
    }
    char perms[10];
    perms[0]=(st.st_mode & S_IRUSR) ? 'r' : '-';
    perms[1]=(st.st_mode & S_IWUSR) ? 'w' : '-';
    perms[2]=(st.st_mode & S_IXUSR) ? 'x' : '-';

    perms[3]=(st.st_mode & S_IRGRP) ? 'r' : '-';
    perms[4]=(st.st_mode & S_IWGRP) ? 'w' : '-';
    perms[5]=(st.st_mode & S_IXGRP) ? 'x' : '-';

    perms[6]=(st.st_mode & S_IROTH) ? 'r' : '-';
    perms[7]=(st.st_mode & S_IWOTH) ? 'w' : '-';
    perms[8]=(st.st_mode & S_IXOTH) ? 'x' : '-';

    perms[9]='\0';

    printf("Permissionhs: %s\n",perms);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modification: %s", ctime(&st.st_mtime));
    close(f);
}

void view(const char *district,int report_id) {
    REPORT r;
    char path[PATH_SIZE];
    int f;

    snprintf(path,sizeof(path),"%s/reports.dat",district);
    f=open(path,O_RDONLY);
    if (f==-1) {
        perror("Error opening reports.dat file!\n");
        return;
    }

    lseek(f, (report_id - 1) * sizeof(REPORT), SEEK_SET);
        if (read(f, &r, sizeof(REPORT)) >0) {
            printf("ID: %d\n", r.id);
            printf("Inspector: %s\n", r.inspectorName);
            printf("GPS: %.2f, %.2f\n", r.gps.latitude, r.gps.longitude);
            printf("Category: %s\n", r.category);
            printf("Severity: %d\n", r.severity);
            printf("Timestamp: %s", ctime(&r.timestamp));
            printf("Description: %s\n", r.description);
            printf("-------------------\n");
        }
        else {
            printf("Raportul cu ID %d nu eixsta in districtul %s\n",report_id,district);
        }
    close(f);
}

/*int check_permissions(const char *path, const char *role, char action) {
    struct stat st;
    if (stat(path, &st) == -1) return 0;

    // Managerii sunt proprietari (User bits), Inspectorii sunt grupul (Group bits)
    if (strcmp(role, "manager") == 0) {
        if (action == 'r') return (st.st_mode & S_IRUSR);
        if (action == 'w') return (st.st_mode & S_IWUSR);
    } else if (strcmp(role, "inspector") == 0) {
        if (action == 'r') return (st.st_mode & S_IRGRP);
        if (action == 'w') return (st.st_mode & S_IWGRP);
    }
    return 0;
}*/
int check_permissions(const char *path,const char *role,char action) {
    struct stat st;
    if (stat(path,&st)==-1) {
        perror("Error stat\n");
        return 0;
    }

    char perms[10];
    perms[0]=(st.st_mode & S_IRUSR) ? 'r' : '-';
    perms[1]=(st.st_mode & S_IWUSR) ? 'w' : '-';
    perms[2]=(st.st_mode & S_IXUSR) ? 'x' : '-';

    perms[3]=(st.st_mode & S_IRGRP) ? 'r' : '-';
    perms[4]=(st.st_mode & S_IWGRP) ? 'w' : '-';
    perms[5]=(st.st_mode & S_IXGRP) ? 'x' : '-';

    perms[6]=(st.st_mode & S_IROTH) ? 'r' : '-';
    perms[7]=(st.st_mode & S_IWOTH) ? 'w' : '-';
    perms[8]=(st.st_mode & S_IXOTH) ? 'x' : '-';

    perms[9]='\0';

    //manager=owner -> primii 3 biti
    //inspector=group ->urmatorii  biti
    if (strcmp(role,"manager")==0) {
        if (action == 'r') return perms[0]=='r';
        if (action == 'w') return perms[1]=='w';
    }
    else {
        if (action == 'r') return perms[3]=='r';
        if (action == 'w') return perms[4]=='w';
    }
    return 0;
}


void update_threshold(const char *district,int value) {
    char path[PATH_SIZE];
    int f;
    snprintf(path,sizeof(path),"%s/district.cfg",district);
    f=open(path,O_WRONLY | O_TRUNC, 0640);
    if (f==-1) {
        perror("Error opening district.cfg file!\n");
        return;
    }
    char buff[64];
    snprintf(buff,sizeof(buff),"Threshold=%d\n",value);
    write(f,buff,strlen(buff));
    close(f);
    printf("Threshold actualizat la %d\n",value);
}
int main(int argc,char *argv[]) {
    if (argc <5) {
        printf("Too few arguments\n");
        return 1;
    }

    char *role=argv[2];
    char *user=argv[4];
    char *command=argv[5];
    char *district=argv[6];
    struct stat st;

    char path[PATH_SIZE];
    snprintf(path,sizeof(path),"%s/reports.dat",district);

    if (strcmp(command,"--add")==0) {
        add_report(district,user);
        add_logged_district(district,user,role,"add");
    }
    else if (strcmp(command,"--list")==0) {
        if (check_permissions(path,role,'r')) {
            list(district);
            add_logged_district(district,user,role,"list");
        }
        else {
           perror("Error: Current role doesn't have permission to read");
        }
    }
    /*else if (strcmp(command,"--view")==0) {
        if (check_permissions(path,role,"r")) {

        }*/
    return 0;
}
