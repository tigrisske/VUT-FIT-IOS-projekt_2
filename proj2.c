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
    int ox_num_inq;
    int OX;
    int max_mol;
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
    sem_t begin;
    sem_t out;
    sem_t wait_till_all_in_q;

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
        //if (atom == 'O'){shared->ox_num_inq++;}
        shared->ox_num_inq++;
        if (shared->ox_num_inq == shared->OX){
            for (int i = 0 ; i< shared->OX; i++){
                sem_post(&shared->wait_till_all_in_q);
            }
        }
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

    if (shared->hyd_num < 2 || shared->ox_num < 1){
        sem_post(&shared->ox);
        //TODO
        if (shared->ox_num_inq != (shared->ox_num+ shared->hyd_num)){
            sem_wait(&shared->wait_till_all_in_q);
        }
        print_to_file(shared,id,atom,4);
        //sem_post(&shared->mutex3);
        //sem_post(&shared->mutex3);


        exit(EXIT_SUCCESS);
    }


    sem_wait(&shared->begin);
    sem_wait(&shared->begin);

    sem_post(&shared->hyd);
    sem_post(&shared->hyd);



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

    sem_wait(&shared->mutex);
    if (shared->hyd_num < 2 || shared->ox_num < 1){
        if (shared->ox_num_inq != (shared->ox_num+ shared->hyd_num)){
            sem_wait(&shared->wait_till_all_in_q);
        }
    //if(shared->max_mol <= shared->mol_num){
        print_to_file(shared,id,atom,5);
        //sem_post(&shared->mutex2);
        sem_post(&shared->mutex);
        exit(EXIT_SUCCESS);
    }

    sem_post(&shared->begin);
    sem_wait(&shared->hyd);


    //creating
    print_to_file(shared, id,atom, 2);

    sem_post(&shared->mutex2);
    sem_wait(&shared->mutex3);

    rand_sleep(TB);
    print_to_file(shared, id, atom, 3);
    sem_post(&shared->mutex4);
    sem_wait(&shared->mutex5);
    //toto tu podla mna nemusi byt
    /*
    if(shared->ox_num == 0){
        sem_post(&shared->hyd);
        sem_post(&shared->hyd);
    }*/
    sem_post(&shared->mutex);
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv){

    //allocating the shared memory
    shared_t *shared;
    MMAP(shared);
    //init all semaphores
    sem_init(&(shared->out), 1, 1);

    //haha
    sem_init(&(shared->mutex), 1, 2);
    sem_init(&(shared->mutex2), 1, 0);
    sem_init(&(shared->mutex3), 1, 0);
    sem_init(&(shared->mutex4), 1, 0);
    sem_init(&(shared->mutex5), 1, 0);
    sem_init(&(shared->begin), 1, 0);
    sem_init(&(shared->ox),1,1);
    sem_init(&(shared->hyd),1,0);
    sem_init(&(shared->wait_till_all_in_q),1,0);

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
    shared->OX = atoi(argv[1]) + atoi(argv[2]);
    shared->hyd_num = atoi(argv[2]);
    shared->max_mol = hyd_num/ox_num;

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

    //forking oxygen
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
    sem_destroy(&(shared->mutex));
    sem_destroy(&(shared->mutex2));
    sem_destroy(&(shared->mutex3));
    sem_destroy(&(shared->mutex4));
    sem_destroy(&(shared->mutex5));
    sem_destroy(&(shared->ox));
    sem_destroy(&(shared->hyd));
    sem_destroy(&(shared->begin));
    sem_destroy(&(shared->wait_till_all_in_q));
    fclose(shared->file_op);
    UNMAP(shared);

    exit(0);
}