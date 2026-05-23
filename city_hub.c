#include "library.h"
void start_monitor() {
    int pfd[2];
    if (pipe(pfd)<0){
        perror("Eroare la crearea pipeului\n");
        exit(1);
    }

    int pid=fork();
    if (pid<0) {
        perror("Eroare la fork hub_mon\n");
        exit(1);
    }

    if (pid==0) {
        //hub_mon
        int monitor_pid=fork();
        if (monitor_pid<0) {
            close(pfd[1]);//inchide scrierea
            exit(1);
        }
        else if (monitor_pid==0) {
            //suntem in procesul care devine monitor_reports
            //redirectionam stdout catre pipe
            close(pfd[0]);//inchide capatul de citire, monitorul doar scrie
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[1]);

            execlp("./monitor_reports","./monitor_reports",NULL);
            perror("Exec monitor failed");
            exit(1);
        }
        //inapoi in hub_mon
        close(pfd[1]);//inchidem capatul de scriere,hub doar citeste

        //citire din pipe
        char buffer[256];
        int bytes;
        while ((bytes=read(pfd[0],buffer,sizeof(buffer)-1)) >0) {
            buffer[bytes]='\0';
            printf("\n[Mesaj monitor]: %s",buffer);

            fflush(stdout);//goleste bufferul si forteaza afisarea imediata a mesajului
            if (strstr(buffer,"[EROARE]")!=NULL)
                break;
        }
        close(pfd[0]);
        waitpid(monitor_pid,NULL,0);//evitam procesele zombie
        exit(0);
    }
    //city_hub
    close(pfd[0]); // parintele(city_hub) nu citeste din pipeul monitorului
    close(pfd[1]); // parintele nu scrie în pipeul monitorului
    printf("[HUB] Monitorul a fost lansat in fundal.PID hub_mon: %d\n",pid);
}

void calculate_scores(char *districts[],int nr) {
    if (nr==0) {
        printf("[HUB] Nu a fost specificat niciun district.\n");
        return;
    }

    int pfds[nr][2]; //cate un pipe pt fiecare district
    int pids[nr];

    //golim bufferul stdout inainte de fork pentru a preveni duplicarea textului "city_hub> "
    fflush(stdout);

    for (int i=0;i<nr;i++) {
        if (pipe(pfds[i])<0) {
            perror("Eroare pipe scorer.\n");
            continue;
        }

        pids[i]=fork();
        if (pids[i]<0) {
            perror("Eroare fork scorer.");
            close(pfds[i][0]);
            close(pfds[i][1]);
            continue;
        }

        if (pids[i]==0) { //copil(scorer)
            close(pfds[i][0]);
            dup2(pfds[i][1], STDOUT_FILENO);
            close(pfds[i][1]);

            execlp("./scorer","./scorer",districts[i],NULL);
            perror("Exec scorer failed");
            exit(1);
        }
        close(pfds[i][1]); //parintele nu scrie
    }

    ScorInspector total[100];
    int nr_total=0;

    for (int i=0;i<nr;i++){
        char buffer[256];
        int bytes;
        while ((bytes=read(pfds[i][0],buffer,sizeof(buffer)-1))  >0) {
            buffer[bytes]='\0';

            // parcurgem bufferul linie cu linie
            char *linie = strtok(buffer, "\n");
            while (linie != NULL) {
                // sarim peste liniile de header
                if (strstr(linie, "---") != NULL) {
                    linie = strtok(NULL, "\n");
                    continue;
                }

                char name[NAME];
                int score;
                if (sscanf(linie, "Inspector: %s\tScor total: %d", name, &score) == 2) {
                    // cautam daca inspectorul exista deja
                    int gasit = 0;
                    for (int j = 0; j < nr_total; j++) {
                        if (strcmp(total[j].name, name) == 0) {
                            total[j].score += score;
                            gasit = 1;
                            break;
                        }
                    }
                    if (gasit == 0) {
                        strcpy(total[nr_total].name, name);
                        total[nr_total].score = score;
                        nr_total++;
                    }
                }
                linie = strtok(NULL, "\n");
            }
        }
        close(pfds[i][0]);
    }
    //curățăm procesele zombie de scoreri după ce le-am citit datele
    for (int i = 0; i < nr; i++) {
        waitpid(pids[i], NULL, 0);
    }
    printf("---Raport combinat---\n");
    for (int i = 0; i < nr_total; i++) {
        printf("Inspector: %5s\tScor total: %d\n", total[i].name, total[i].score);
    }

}
int main() {
    char command[50];
    while (1) {
        printf("city_hub> ");
        fgets(command,sizeof(command),stdin);
        command[strcspn(command,"\n")]='\0';

        if (strlen(command)==0)//pt enter
            continue;
        if (strcmp(command,"start_monitor")==0)
            start_monitor();
        else if (strncmp(command,"calculate_scores",16)==0) {
            char *districts[100];
            int nr=0;

            char *district=strtok(command+16," \t");
            while (district!=NULL && nr<100) {
                districts[nr++]=district;
                district=strtok(NULL," \t");
            }

            calculate_scores(districts,nr);
        }
        else if (strcmp(command,"exit")==0)
            break;
        else
            printf("Comanda necunoscuta.\n");
    }
    return 0;
}