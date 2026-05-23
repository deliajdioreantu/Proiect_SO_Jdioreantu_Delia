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

typedef struct ScorInspector{
    char name[NAME];
    int score;
}ScorInspector;
