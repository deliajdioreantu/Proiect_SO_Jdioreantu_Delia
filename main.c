#include <stdio.h>
#include <sys/stat.h>
#include<sys/wait.h>
#include<unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include<time.h>
#include<signal.h>

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
    if (mkdir(district, 0750) == -1) { //daca exista deja
        if (errno != EEXIST) {
            perror("Error mkdir");
            return;
        }
    }
    // Setare explicita pentru siguranta
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
        chmod(path, 0640);
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

    struct stat lst;
    if (lstat(link_name,&lst)==0) {
        //exista ceva cu numele asta
        struct stat st;
        if (stat(link_name,&st)==-1) { //esuat,dangling link
            printf("Warning! Dangling symlink %s\n",link_name);
            unlink(link_name); //sterge symlink-ul vechi
            symlink(path,link_name);
        }
    }
    else {
        symlink(path,link_name); //daca nu exista deloc,il creem
    }
    //dangling link-daca s a sterg fisierul din path,symlinkul continue sa existe
}

void add_report(const char *district,const char *user) {
    REPORT r;
    char path[PATH_SIZE];
    int f;

    printf("X: "); scanf("%f", &r.gps.latitude);
    printf("Y: "); scanf("%f", &r.gps.longitude);

    printf("Category (road/lighting/flooding/other): ");
    scanf("%s",r.category);

    printf("Severity level (1/2/3): ");
    scanf("%d",&r.severity);

    getchar(); // Consumă newline-ul rămas în buffer pentru ca fgets să funcționeze
    printf("Description: ");
    fgets(r.description, DESCR, stdin);
    r.description[strcspn(r.description, "\n")] = 0;

    snprintf(path, sizeof(path), "%s/reports.dat", district);
    f=open(path,O_RDWR,0664);
    if (f == -1) {
        perror("Open error reports.dat");
        return;
    }

    struct stat st;
    fstat(f, &st);
    if (st.st_size == 0) {
        r.id=1; //primul raport
    }
    else {
        REPORT ultimul;
        lseek(f,-sizeof(REPORT),SEEK_END);
        read(f,&ultimul,sizeof(REPORT));
        r.id= ultimul.id+1;
    }

    strncpy(r.inspectorName, user, NAME-1);
    r.timestamp=time(NULL);

    lseek(f,0,SEEK_END);
    if (write(f, &r, sizeof(REPORT)) == -1) {
        perror("Error saving report!\n");
    } else {
        printf("Raport saved with ID: %d\n", r.id);
    }
    close(f);
}

void add_logged_district(const char *district,const char *user, const char *role,const char *action){
    char path[PATH_SIZE];
    char log[LOG_SIZE];
    int f;

    snprintf(path,sizeof(path),"%s/logged_district", district);
    struct stat st;
    if (stat(path,&st)==-1) {
        perror("Stat error on logged_district");
        return;
    }
    //verifica daca managerul(USR) are dreptul sa scrie(W)
    if ((st.st_mode & S_IWUSR)== 0) {
        fprintf(stderr,"Error:no permission to write on logged_district\n");
        return;
    }

    f=open(path,O_WRONLY| O_APPEND,0644);
    if (f == -1) {
        perror("Open error logged_district");
        return;
    }

    time_t now=time(NULL);
    snprintf(log,sizeof(log),"%s\t%ld\t%s\t%s\n",action,(long)now,role,user);
    write(f,log,strlen(log)); //ca sa scrie doar caracterele utile,exact cate sunt in log
    close(f);
}
void  permissions_to_string(mode_t mode,char *perms) {
    perms[0]=(mode & S_IRUSR) ? 'r' : '-';
    perms[1]=(mode & S_IWUSR) ? 'w' : '-';
    perms[2]=(mode & S_IXUSR) ? 'x' : '-';

    perms[3]=(mode & S_IRGRP) ? 'r' : '-';
    perms[4]=(mode & S_IWGRP) ? 'w' : '-';
    perms[5]=(mode & S_IXGRP) ? 'x' : '-';

    perms[6]=(mode & S_IROTH) ? 'r' : '-';
    perms[7]=(mode & S_IWOTH) ? 'w' : '-';
    perms[8]=(mode & S_IXOTH) ? 'x' : '-';

    perms[9]='\0';
}
void list(const char *district) {
    REPORT r;
    char path[PATH_SIZE];
    int f;
    snprintf(path,sizeof(path),"%s/reports.dat",district);
    f=open(path,O_RDONLY);
    if (f==-1) {
        fprintf(stderr,"Error opening reports.dat file!\n");
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
    if (stat(path, &st) == -1) {
        perror("Eroare la citirea atributelor fisierului");
        close(f);
        return;
    }
    char perms[10];
    permissions_to_string(st.st_mode, perms);

    printf("File informations:\n");
    printf("Permissions: %s\n",perms);
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
        fprintf(stderr,"Error opening reports.dat file!\n");
        return;
    }

    int found = 0;
    while (read(f, &r, sizeof(REPORT)) > 0) {
        if (r.id == report_id) {
            printf("ID: %d\n", r.id);
            printf("Inspector: %s\n", r.inspectorName);
            printf("GPS: %.2f, %.2f\n", r.gps.latitude, r.gps.longitude);
            printf("Category: %s\n", r.category);
            printf("Severity: %d\n", r.severity);
            printf("Timestamp: %s", ctime(&r.timestamp));
            printf("Description: %s\n", r.description);
            printf("-------------------\n");
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Raportul cu ID %d nu exista in districtul %s\n", report_id, district);
    }
    close(f);
}

int check_permissions(const char *path, const char *role, char action) {
    struct stat st;
    if (stat(path, &st) == -1) return 0;

    // Managerul e user, inspectorii sunt grupul
    if (strcmp(role, "manager") == 0) {
        if (action == 'r') return (st.st_mode & S_IRUSR);
        if (action == 'w') return (st.st_mode & S_IWUSR);
    } else if (strcmp(role, "inspector") == 0) {
        if (action == 'r') return (st.st_mode & S_IRGRP);
        if (action == 'w') return (st.st_mode & S_IWGRP);
    }
    return 0;
}

void remove_report(const char *district,int raport_id)
{
    char path[PATH_SIZE];
    snprintf(path,sizeof(path),"%s/reports.dat",district);

    int f=open(path,O_RDWR);
    if (f==-1)
    {
        perror("Eroare la deschiderea reports.dat pentru ștergere\n");
        return;
    }

    struct stat st;
    if (fstat(f,&st) == -1)
    {
        perror("Eroare la fstat");
        close(f);
        return;
    }

    REPORT r;
    int index_de_sters=-1;
    int i=0;

    while (read(f,&r,sizeof(REPORT))>0)
    {
        if (r.id==raport_id)
        {
            index_de_sters=i;
            break;
        }
        i=i+1;
    }

    if (index_de_sters == -1)
    {
        printf("Raportul cu ID %d nu a fost găsit în districtul %s!\n", raport_id, district);
        close(f);
        return;
    }

    int total=st.st_size/sizeof(REPORT);

    for (i=index_de_sters+1;i<total;i++)
    {
        lseek(f,i*sizeof(REPORT),SEEK_SET);
        read(f,&r,sizeof(REPORT));

        lseek(f,(i - 1)*sizeof(REPORT), SEEK_SET);
        write(f,&r,sizeof(REPORT));
    }

    if (ftruncate(f,(total-1)*sizeof(REPORT))==-1)
    {
        perror("Eroare la stergere ultimul rand (cel dublificat)\n");
    }
    close(f);
}

void update_threshold(const char *district,int value) {
    char path[PATH_SIZE];
    snprintf(path,sizeof(path),"%s/district.cfg",district);

    struct stat st;
    if (stat(path,&st)==-1) {
        perror("Stat failed on distrct.cfg");
        return;
    }

    if ( (st.st_mode & 0777)!= 0640) {
        fprintf(stderr,"Error! The permissions of district.cfg have been modified\n");
        return;
    }

    int f=open(path,O_WRONLY | O_TRUNC, 0640);
    if (f==-1) {
        fprintf(stderr,"Error opening district.cfg file!\n");
        return;
    }
    char buff[64];
    snprintf(buff,sizeof(buff),"Threshold=%d\n",value);
    write(f,buff,strlen(buff));
    printf("Threshold updated: %d\n",value);
    close(f);
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    // copiem input-ul ca sa nu il modificam
    char copy[100];
    strncpy(copy, input, sizeof(copy)-1);

    // strtok sparge string-ul dupa ":"
    char *token = strtok(copy, ":");
    if (token == NULL) return 0;
    strcpy(field, token);  // ex: "severity"

    token = strtok(NULL, ":");
    if (token == NULL) return 0;
    strcpy(op, token);     // ex: ">="

    token = strtok(NULL, ":");
    if (token == NULL) return 0;
    strcpy(value, token);  // ex: "2"

    return 1;  // succes
}

int match_condition(REPORT *r, const char *field, const char *op, const char *value) {

    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);  // convertim "2" la int
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<")  == 0) return r->severity <  val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">")  == 0) return r->severity >  val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    }

    if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    }

    if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspectorName, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    }

    if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);  // timestamp e long
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, "<")  == 0) return r->timestamp <  val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">")  == 0) return r->timestamp >  val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
    }

    return 0;  // field necunoscut
}

void filter(const char *district,int argc, char *argv[]) {
    char path[PATH_SIZE];
    snprintf(path,sizeof(path),"%s/reports.dat",district);
    int f=open(path, O_RDONLY);
    if (f==-1) {
        perror("Open failed");
        return;
    }

    REPORT r;
    int nr=0;
    while ( read(f, &r, sizeof(REPORT)) > 0) {
        int match=1; // presupunem ca raportul se potriveste

        for (int i=7; i<argc;i++) {
            char field[50], op[10], value[50];
            parse_condition(argv[i], field, op, value);

            if (match_condition(&r, field, op, value) ==0) {
                match=0; // o conditie nu e indeplinita, ne oprim nu mai verificam celelalte
                break;
            }
        }

        if (match==1) {
            printf("ID: %d\n", r.id);
            printf("Inspector: %s\n", r.inspectorName);
            printf("GPS: %.2f, %.2f\n", r.gps.latitude, r.gps.longitude);
            printf("Category: %s\n", r.category);
            printf("Severity: %d\n", r.severity);
            printf("Timestamp: %s", ctime(&r.timestamp));
            printf("Description: %s\n", r.description);
            printf("-------------------\n");
            nr++;
        }
    }
    if (nr==0) {
        printf("Niciun raport nu indeplineste conditiile.\n");
    }
    close(f);
}

void remove_district(const char *district) {
    char link_name[LINK_SIZE];
    snprintf(link_name, sizeof(link_name),"active_reports-%s",district);
    unlink(link_name);

    pid_t pid=fork();
    if (pid<0) {
        perror("Fork failed");
        exit(1);
    }
    if (pid==0) { //codul fiului
        execlp("rm", "rm","-rf",district,NULL);
        perror("Exec failed");
        exit(1);
    }
    //cod parinte
    wait(NULL);
    printf("District %s removed.\n",district);

}

void notify_monitor(const char *district,const char *user,const char *role) {
    int monitor_informed=0;
    int f=open(".monitor_pid",O_RDONLY);
    if (f!=-1) {
        char string_pid[10];

        int bytes=read(f,string_pid,sizeof(string_pid)-1);
        close(f);

        if (bytes>0) {
            string_pid[bytes]='\0';
            pid_t monitor_pid=atoi(string_pid);

            if (kill(monitor_pid, SIGUSR1)==0)
                monitor_informed=1;
        }
    }

    char text[100];
    if (monitor_informed!=0) {
        snprintf(text,sizeof(text), "ADD REPORT- MONITOR INFORMED");
        printf("Monitorul a fost informat cu succes.\n");
    }
    else {
        snprintf(text,sizeof(text),"ADD REPORT- MONITOR COULD NOT BE INFORMED");
        fprintf(stderr,"Eroare: Monitorul nu a putut fi informat!\n");
    }

    add_logged_district(district,user,role,text);
}




int main(int argc,char *argv[]) {
    if (argc <7) {
        printf("Too few arguments\n");
        return 1;
    }

    char *role=argv[2];
    char *user=argv[4];
    char *command=argv[5];
    char *district=argv[6];


    //create(district);
    if (strcmp(role, "manager")!=0 && strcmp(role,"inspector") != 0) {
        printf("Invalid arguments\n");
        return 1;
    }
    char path_reports[PATH_SIZE];
    char path_cfg[PATH_SIZE];
    snprintf(path_reports,sizeof(path_reports),"%s/reports.dat",district);
    snprintf(path_cfg,sizeof(path_cfg),"%s/district.cfg",district);

    create(district);

    if (strcmp(command,"--add")==0) {
        if (check_permissions(path_reports,role,'w')) {
            add_report(district,user);
            notify_monitor(district,user,role);
        }
        else {
            fprintf(stderr,"Error: Current role doesn't have permission to write\n");
        }
    }

    else if (strcmp(command,"--list")==0) {
        if (check_permissions(path_reports,role,'r')) {
            list(district);
            if (strcmp(role,"manager")==0)
                add_logged_district(district,user,role,"list");
        }
        else {
            fprintf(stderr,"Error: Current role doesn't have permission to read");
        }
    }

    else if (strcmp(command,"--view")==0) {
        if (check_permissions(path_reports,role,'r')) {

            if (argc<8) {
                printf("Command %s should contain report id\n",command);
                exit(1);
            }

            view(district,atoi(argv[7]));
            if (strcmp(role,"manager")==0)
                add_logged_district(district,user,role,"view");
        }
        else {
            fprintf(stderr,"Error: Current role doesn't have permission to read");
        }
    }

    else if (strcmp(command,"--update_threshold")==0) {

        if (strcmp(role,"manager")==0) {
            if ( check_permissions(path_cfg,role,'w')) {

                if (argc<8) {
                    printf("Command %s should contain value of the new threshold.\n",command);
                    exit(1);
                }
                update_threshold(district,atoi(argv[7]));
                add_logged_district(district,user,role,"update_threshold");
            }
        }
        else {
            fprintf(stderr,"Manager role only\n");
        }
    }

    else if (strcmp(command,"--remove_report")==0) {
        if (strcmp(role,"manager")==0) {
            if ( check_permissions(path_reports,role,'w')) {
                if (argc<8) {
                    printf("Command %s should contain report id.\n",command);
                    exit(1);
                }

                remove_report(district,atoi(argv[7]));
                add_logged_district(district,user,role,"remove report");
            }
        }
        else {
            fprintf(stderr,"Manager role only\n");
        }
    }

    else if (strcmp(command,"--filter")==0) {
        if (check_permissions(path_reports,role,'r')) {

            if (argc<8) {
                printf("Command %s should contain at least one condition.\n",command);
                exit(1);
            }

            filter(district,argc,argv);
            if (strcmp(role,"manager")==0)
                add_logged_district(district,user,role,"filter");
        }
        else {
            fprintf(stderr,"Error: Current role doesn't have permission to read");
        }
    }

    else if (strcmp(command,"--remove_district")==0) {
        if (strcmp(role,"manager")==0) {
            remove_district(district);
        }
        else {
            fprintf(stderr,"Error: Current role doesn't have permission.\n");
        }
    }

    return 0;
}