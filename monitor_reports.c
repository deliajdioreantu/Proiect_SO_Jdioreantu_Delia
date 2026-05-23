#include "library.h"
#define PID_FILE ".monitor_pid"

void signal_handler(int sig) {
    if (sig==SIGUSR1) {
        char text[100]="[INFO] Semnal SIGUSR1! A fost adaugat un nou raport!\n";
        write(1,text,strlen(text));
    }
    else if (sig==SIGINT) {
        char text[100]="[INFO] Semnal SIGINT! Se inchide monitorul\n";
        write(1,text,strlen(text));

        unlink(PID_FILE);
        exit(0);
    }
}

int main() {
    int check=open(PID_FILE,O_RDONLY);
    if ( check!=-1) {
        char pid[10];
        int bytes=read(check, pid,sizeof(pid)-1);
        close(check);
        if (bytes >0) {
            pid[bytes]='\0';
            char mesaj_err[100];
            int len = snprintf(mesaj_err, sizeof(mesaj_err), "[EROARE] Monitor activ cu PID %s.\n", pid);
            write(STDOUT_FILENO, mesaj_err, len);
            exit(1);
        }
    }

    int f=open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f==-1) {
        perror("Eroare la crearea .monitor_pid");
        exit(1);
    }

    char pid_string[10];
    int len=snprintf(pid_string, sizeof(pid_string),"%d",getpid());
    write(f, pid_string, len);
    close(f);

    char mesaj[100];
    len=snprintf(mesaj, sizeof(mesaj),"[INFO] Monitor pornit cu succes: PID %d.\n", getpid());
    write(STDOUT_FILENO, mesaj, len);

    struct sigaction s;
    s.sa_handler=signal_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags=SA_RESTART;

    if (sigaction(SIGUSR1, &s, NULL) == -1)
        perror("Error sigaction SIGUSR1");
    if (sigaction(SIGINT, &s, NULL) == -1)
        perror("Error sigaction SIGINT");

    while (1) {
        pause();
    }
    return 0;
}