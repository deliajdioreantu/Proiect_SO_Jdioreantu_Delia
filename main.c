#include <stdio.h>
#include <time.h>
#include<unistd.h>
#include<string.h>

#define SIZE 100
typedef struct {
    float latitude;
    float longitude;
}GPS;
typedef struct {
    int id;
    char inspectorName[SIZE];
    GPS gps;
    char category[SIZE];
    int severity;
    time_t timestamp;
    char description[SIZE];
}REPORT;



int main(int argc,char *argv[]) {
    if (argc < 3) {
        char text[]="Too few arguments";
        write(2,text,sizeof(text));
        return ;
    }

    if (strcmp(argv[2],"--role")!=0) {
        char text[]="Wrong syntax";
        write(2,text,sizeof(text));
        return ;
    }
    return 0;
}