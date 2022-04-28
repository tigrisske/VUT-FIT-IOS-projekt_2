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
const char *mol_creating = "creating molecule";
const char *mol_created = "molecule created";

typedef struct shared{
    int hyd_num ;
    int ox_num;
    int mol_num;
    int atoms_in;
    char H1;
    char H2;
    size_t rows_cnt;

    //semaphores
    sem_t hyd;
    sem_t ox;
    sem_t mutex;
    sem_t mutex2;
    sem_t mutex3;
    sem_t mutex4;
    sem_t mutex5;
    sem_t out;
    FILE* file_op;

}shared_t;

//print function
void print_to_file(shared_t *shared, int id, char atom, int todo){
    //only one process can write to file at the time
    sem_wait(&shared->out);

    //started
    if (todo == 0) {
        //if(atom == 'O'){shared->mol_num++;}
        fprintf(shared->file_op, "%ld: %c %d: started\n", ++shared->rows_cnt, atom, id);
        fflush(shared->file_op);

    }
    //going to queue
    else if (todo == 1){
        fprintf(shared->file_op, "%ld: %c %d: going to queue\n", ++shared->rows_cnt, atom, id);
        //if (atom == 'O'){shared->ox_num--;}
        //if (atom == 'H'){shared->hyd_num--;}
        fflush(shared->file_op);
    }
    //creating molecule
    else if (todo == 2){
        fprintf(shared->file_op, "%ld: %c %d: creating molecule %d\n", ++shared->rows_cnt, atom, id, shared->mol_num);
        fflush(shared->file_op);


    }
    //molecule created
    else if (todo == 3){
        //TODO neni dobre
        shared->atoms_in++;
        fprintf(shared->file_op, "%ld: %c %d: molecule %d created\n", ++shared->rows_cnt, atom, id, shared->mol_num );
        if (shared->atoms_in == 3){shared->mol_num++;shared->atoms_in = 0;}
        if (atom == 'O'){shared->ox_num--;}
        if (atom == 'H'){shared->hyd_num--;}
        fflush(shared->file_op);
        //shared->atoms_in--;
    }
    else if( todo == 4){
        fprintf(shared->file_op, "%ld: %c %d: not enough H\n",++shared->rows_cnt, atom, id);
        fflush(shared->file_op);
    }
    else if( todo == 5){
        fprintf(shared->file_op, "%ld: %c %d: not enough O or H\n",++shared->rows_cnt, atom, id);
        fflush(shared->file_op);
    }
    //allowing other processes to access output file
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
    //shared->atoms_in++;
    /*
    if (shared->atoms_in == 3){
        shared->mol_num++;
    }
     */
    //creating molecule
    print_to_file(shared, id,atom, 2);
    //printf("%d: %c %d: creating molecule %d\n", ++shared->rows_cnt, atom, id, ++shared->mol_num);
    rand_sleep(time);
    print_to_file(shared, id, atom, 3);


    if (shared->atoms_in ==3) {
        sem_post(&shared->mutex);
    }
    else{
        sem_wait(&shared->mutex);
    }
    sem_post(&(shared->hyd));
    sem_post(&(shared->hyd));
    sem_post(&(shared->ox));
    shared->atoms_in = 0;
}


//function that creates oxygen
void oxygen(int id, int TI,int TB, shared_t *shared){
    char atom = 'O';
    //starting atom
    print_to_file(shared,id,atom,  0);
    //putting atom to sleep
    rand_sleep(TI);
    //printing out atom going to queue
    print_to_file(shared,id,atom, 1);

    sem_wait(&shared->ox);

    sem_post(&shared->hyd);
    sem_post(&shared->hyd);

    if (shared->hyd_num < 2 || shared->ox_num < 1){
        print_to_file(shared,id,atom,4);
        sem_post(&shared->mutex3);
        sem_post(&shared->mutex3);
        sem_post(&shared->ox);
        exit(EXIT_SUCCESS);
    }

    print_to_file(shared, id,atom, 2);

    sem_wait(&shared->mutex2);
    sem_wait(&shared->mutex2);

    sem_post(&shared->mutex3);
    sem_post(&shared->mutex3);

    //sleeping and created
    rand_sleep(TB);
    print_to_file(shared, id, atom, 3);


    sem_wait(&shared->mutex4);
    sem_wait(&shared->mutex4);

    sem_post(&shared->mutex5);
    sem_post(&shared->mutex5);

    sem_post(&shared->ox);
    exit(EXIT_SUCCESS);

}

//function that creates hydrogen
void hydrogen(int id, int TI,int TB, shared_t *shared) {
    char atom = 'H';
    //printing out that atom started
    print_to_file(shared,id,atom,0);

    //putting process to sleep
    rand_sleep(TI);
    //going to queue after sleep
    print_to_file(shared,id,atom,1);



    sem_wait(&shared->hyd);
    if (shared->hyd_num < 2 || shared->ox_num < 1){
        print_to_file(shared,id,atom,5);
        sem_post(&shared->mutex2);
        exit(EXIT_SUCCESS);
    }

    //creating
    print_to_file(shared, id,atom, 2);

    sem_post(&shared->mutex2);
    sem_wait(&shared->mutex3);

    rand_sleep(TB);
    print_to_file(shared, id, atom, 3);
    sem_post(&shared->mutex4);
    sem_wait(&shared->mutex5);
    if(shared->ox_num == 0){
        sem_post(&shared->hyd);
        sem_post(&shared->hyd);
    }
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv){

    //allocating the shared memory
    shared_t *shared;
    MMAP(shared);
    //init all semaphores
    sem_init(&(shared->out), 1, 1);

    //haha
    sem_init(&(shared->mutex2), 1, 0);
    sem_init(&(shared->mutex3), 1, 0);
    sem_init(&(shared->mutex4), 1, 0);
    sem_init(&(shared->mutex5), 1, 0);
    sem_init(&(shared->ox),1,1);
    sem_init(&(shared->hyd),1,0);
    shared->file_op =  fopen("proj2.out", "w");

    //shared->ox_num = 0;
    //shared->hyd_num = 0;
    shared->atoms_in = 0;
    shared->mol_num = 1;

    //checking parameters
    if (argc != ARG_NUM){
        fprintf(stderr,"[ERROR] invalid amount of arguments given.\n");
        exit(EXIT_FAILURE);
    }

    int NO = atoi(argv[1]);
    int NH = atoi(argv[2]);
    int TI = atoi(argv[3]); 
    int TB = atoi(argv[4]); 
    //if given parameters are not in range <0,1000> program exits
    if (!(inRange(0,1000,TI) && inRange(0,1000,TB))){
        fprintf(stderr, "[ERROR] wrong range of given arguments");
        exit(EXIT_FAILURE);
    }

    //CHECKING WHETHER GIVEN ARGUMENTS ARE NUMBERS
    char* block;
    int no = (int)strtol(argv[1], &block, 0);
    if (*block != '\0' || no < 0)
    {
        fprintf(stderr,"[ERROR] arguments must be numeric\n");
        exit(EXIT_FAILURE);
    }

    int nh = (int)strtol(argv[2], &block, 0);
    if (*block != '\0' || nh < 0)
    {
        fprintf(stderr,"[ERROR] arguments must be numeric\n");
        exit(EXIT_FAILURE);
    }

    int ti = (int)strtol(argv[3], &block, 0);
    if (*block != '\0' || ti < 0 || ti > 1000)
    {
        fprintf(stderr,"[ERROR] arguments must be numeric\n");
        exit(EXIT_FAILURE);
    }

    int tb = (int)strtol(argv[4], &block, 0);
    if (*block != '\0' || tb < 0 || tb > 1000)
    {
        fprintf(stderr,"[ERROR] arguments must be numeric\n");
        exit(EXIT_FAILURE);
    }

    if (NO <= 0 || NH <= 0){
        fprintf(stderr, "[ERROR] NO and NH can not be negative nor zero values\n");
        exit(EXIT_FAILURE);
    }

    //declaring variables
    int ox_num = atoi(argv[1]);
    int hyd_num = atoi(argv[2]);
    shared->ox_num = atoi(argv[1]);
    shared->hyd_num = atoi(argv[2]);

    pid_t pid;
    //forking hydrogen
    for(int i = 0; i < hyd_num; i++){
        pid = fork();
        if (pid == 0){
            hydrogen(i+1,TI,TB, shared);
            exit(EXIT_SUCCESS);
        }
        else if(pid <0){
            fprintf(stderr, "ERROR creating child process");
            exit(EXIT_FAILURE);
        }
    }

    //forking oxygen brasko
    for(int i = 0; i < ox_num; i++){
        pid = fork();
        if (pid == 0){
            oxygen(i+1, TI,TB, shared);
            exit(EXIT_SUCCESS);
        }
        else if(pid <0){
            fprintf(stderr, "ERROR creating child process");
            exit(EXIT_FAILURE);
        }
    }
    sem_destroy(&(shared->out));

    //haha
    sem_destroy(&(shared->mutex2));
    sem_destroy(&(shared->mutex3));
    sem_destroy(&(shared->mutex4));
    sem_destroy(&(shared->mutex5));
    sem_destroy(&(shared->ox));
    sem_destroy(&(shared->hyd));
    fclose(shared->file_op);
    UNMAP(shared);

    exit(0);
}
