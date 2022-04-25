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

void foo(){
    

}


int main (){
    sem_t sem;
    sem_init(&(sem), 1, 1);

    pid_t pid;
    for (int i = 0; i < 3; i++){
        pid = fork();
        if (pid == 0){
            printf("nasjskor sa stane toto\n");
            sem_post(&sem);
        }
        if (pid >0){
            sem_wait(&sem);
            ("potom toto brasko\n");
            sem_post(&sem);
        }
    }
    return 0;
}

