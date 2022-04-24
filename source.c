#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>



#include <assert.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ARG_NUM 5

//function that puts a process to sleep
void rand_sleep(int time){
    usleep((rand()%(time+1)));
}


bool inRange(int min, int max, int arg){
    if (arg >= min && arg <= max){
        return true;
    }
    return false;
}


int oxygen(int id, int wait_time){
    printf("O %d: stared\n", id);
    rand_sleep(wait_time);
    printf("O %d: going to queue\n", id);
    return 0;
}

int hydrogen(int id, int wait_time) {
    printf("H %d: stared\n", id);
    rand_sleep(wait_time);
    printf("H %d: going to queue\n", id);
    return 0;
}

int main(int argc, char **argv){

    if (argc != ARG_NUM){
        printf("1");
        return 1;
    }

    int TI = atoi(argv[3]); 
    int TB = atoi(argv[4]); 
    //if given parameters are not in range <0,1000> program exits
    if (!(inRange(0,1000,TI) && inRange(0,1000,TB))){
        printf("1");
        return 1;

    }
    //variables
    int ox_num = atoi(argv[2]);
    int hyd_num = atoi(argv[1]);

    pid_t pid;
    //forking hydrogen
    for(int i = 0; i < hyd_num; i++){
        pid = fork();
        if (pid == 0){
            hydrogen(i+1,TI );
            exit(0);
        }
        else if(pid <0){
            printf(stderr, "ERROR creating child process");
            //return 1;
        }
    }

    //forking oxygen brasko
    for(int i = 0; i < ox_num; i++){
        pid = fork();
        if (pid == 0){
            oxygen(i+1, TI);
            exit(0);
        }
        else if(pid <0){
            printf(stderr, "ERROR creating child process");
            //return 1;
        }
    }




    return 0;    
}
