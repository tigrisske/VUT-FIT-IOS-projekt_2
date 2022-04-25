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

#define MMAP(pointer){(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

const char *started = "started";
const char *queue = "going to queue";



typedef struct shared{
    int hyd_num ;
    int ox_num;
    int mol_num;
    size_t rows_cnt;

    //semaphores
    sem_t hyd;
    sem_t ox;
    sem_t mutex;
    sem_t out;
    FILE* file_op;

}shared_t;

//print function
void print_to_file(shared_t *shared, int id, char atom){
    sem_wait(&shared->out);
    //printf("%d\n",++shared->rows_cnt);
    fprintf(shared->file_op ,"%ld: %c %d: started\n", ++shared->rows_cnt, atom,id);
    fflush(shared->file_op);
    sem_post(&shared->out);
}



//function to put the process sleep
void rand_sleep(int time){
    usleep((rand()%(time+1)));
}

//function checks whether given argument is in given range
bool inRange(int min, int max, int arg){
    if (arg >= min && arg <= max){
        return true;
    }
    return false;
}

//molecule function
void create_mol(int time, shared_t *shared, int id, char atom){
    if (shared->hyd_num>=2 && shared->ox_num>=1){
        //print_to_file()
        printf("%d: %c %d: creating molecule %d\n", ++shared->rows_cnt, atom, id, ++shared->mol_num);
        if (atom == 'H'){
            shared->hyd_num--;
        }
        else{
            shared->ox_num--;
        }
    }
}

//function that creates oxygen
void oxygen(int id, int wait_time, shared_t *shared){
    print_to_file(shared,id,'O');
    //printf("%d: O %d: stared\n",++shared->rows_cnt, id);
    rand_sleep(wait_time);
    shared->ox_num++;
    char atom = 'O';
    print_to_file(shared,id,atom);
    //printf("%d: O %d: going to queue\n", ++shared->rows_cnt, id);
    //create_mol(wait_time, shared, id, atom);
    //printf("number of oxygens in queue: %d\n", shared->ox_num);
}

//function that creates hydrogen
void hydrogen(int id, int wait_time, shared_t *shared) {
    print_to_file(shared,id,'H');
    //printf("%d: H %d: stared\n",++shared->rows_cnt, id);
    rand_sleep(wait_time);
    shared->hyd_num++;
    char atom = 'H';
    //printf("%d: H %d: going to queue\n",++shared->rows_cnt, id);
    print_to_file(shared,id,atom);
    //create_mol(wait_time, shared, id, atom);
    //printf("number of oxygens in queue: %d\n", shared->hyd_num);
}


int main(int argc, char **argv){

    //allocating the shared memory
    shared_t *shared;
    MMAP(shared);
    sem_init(&(shared->out), 1, 1);
    shared->file_op =  fopen("proj2.out", "w");

    //checking parameters
    if (argc != ARG_NUM){
        fprintf(stderr,"[ERROR] invalid amount of arguments given.\n");
        return 1;
    }

    int TI = atoi(argv[3]); 
    int TB = atoi(argv[4]); 
    //if given parameters are not in range <0,1000> program exits
    if (!(inRange(0,1000,TI) && inRange(0,1000,TB))){
        printf("1");
        return 1;

    }

    //declaring variables
    int ox_num = atoi(argv[1]);
    int hyd_num = atoi(argv[2]);

    pid_t pid;
    //forking hydrogen
    for(int i = 0; i < hyd_num; i++){
        pid = fork();
        if (pid == 0){
            hydrogen(i+1,TI, shared);
            exit(0);
        }
        else if(pid <0){
            fprintf(stderr, "ERROR creating child process");
            //return 1;
        }
    }

    //forking oxygen brasko
    for(int i = 0; i < ox_num; i++){
        pid = fork();
        if (pid == 0){
            oxygen(i+1, TI, shared);
            exit(0);
        }
        else if(pid <0){
            fprintf(stderr, "ERROR creating child process");
            //return 1;
        }
    }

    return 0;    
}
