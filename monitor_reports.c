#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#define PID_FILE ".monitor_pid"

void signal_handler(int sig) {
    if (sig==SIGUSR1) {
        char text[100]="Semnal SIGUSR1! A fost adaugat un nou raport!\n";
        write(1,text,strlen(text));
    }
    else if (sig==SIGINT) {
        char text[100]="Semnal SIGINT! Se inchide monitorul\n";
        write(1,text,strlen(text));

        unlink(PID_FILE);
        exit(0);
    }
}

int main() {
    int f=open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f==-1) {
        perror("Eroare la crearea .monitor_pid");
        exit(1);
    }

    char pid_string[10];
    int len=snprintf(pid_string, sizeof(pid_string),"%d",getpid());
    write(f, pid_string, len);
    close(f);

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