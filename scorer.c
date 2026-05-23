#include "library.h"
int main(int argc, char *argv[]){
  if(argc<2){
     printf("Scorer error: missing district!\n");
     return 1;
  }

  char path[PATH_SIZE];
  snprintf(path,sizeof(path),"%s/reports.dat",argv[1]);

  int f=open(path,O_RDONLY);
  if(f==-1){
     printf("Error opening reports.dat\n");
     return 1;
  }

  ScorInspector scores[100]={0};
  int nr_inspectori=0;
  REPORT r;

  while(read(f,&r,sizeof(REPORT)) >0){
    int gasit=0;
    for(int i=0;i<nr_inspectori;i++){
      if(strcmp(scores[i].name,r.inspectorName)==0){
         scores[i].score +=r.severity;
         gasit=1;
         break;
      }
    }
    //inspectorul nu exista
    if(gasit==0 && nr_inspectori<100){
      strcpy(scores[nr_inspectori].name,r.inspectorName);
      scores[nr_inspectori].score=r.severity;
      nr_inspectori++;
    }
  }
  close(f);

  printf("---Workload report district %s---\n",argv[1]);
  for(int i=0;i<nr_inspectori;i++){
    printf("Inspector: %s\tScor total: %d\n",scores[i].name,scores[i].score);
  }
  return 0;
}