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
#include <string.h>

#define ARG_NUM 5

#define MMAP(pointer){(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

typedef struct shared{

    //shared variables
    int hyd_num ;
    int ox_num;
    int mol_num;
    int atoms_in;
    int atoms_num_inq;
    int OX_n_HYD;
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

//function that prints into file
void print_to_file(shared_t *shared, int id, char atom, int todo){
    //only one process can write to file at the time
    sem_wait(&shared->out);

    //started
    if (todo == 0) {
        fprintf(shared->file_op, "%ld: %c %d: started\n", ++shared->rows_cnt, atom, id);
        fflush(shared->file_op);

    }
    //going to queue
    else if (todo == 1){
        fprintf(shared->file_op, "%ld: %c %d: going to queue\n", ++shared->rows_cnt, atom, id);
        //counting number of atoms in queue
        shared->atoms_num_inq++;
        //if all atoms are in queue semaphore lets other atoms that wait print "not enough..."
        if (shared->atoms_num_inq == shared->OX_n_HYD){
            for (int i = 0 ; i< shared->OX_n_HYD; i++){
                sem_post(&shared->wait_till_all_in_q);
            }
        }
        fflush(shared->file_op);
    }
    //creating molecule
    else if (todo == 2){
        fprintf(shared->file_op, "%ld: %c %d: creating molecule %d\n", ++shared->rows_cnt, atom, id, shared->mol_num);
        fflush(shared->file_op);


    }
    //molecule created
    else if (todo == 3){
        shared->atoms_in++;//counting atoms that are currently creating a molecule
        fprintf(shared->file_op, "%ld: %c %d: molecule %d created\n", ++shared->rows_cnt, atom, id, shared->mol_num );

        //every three atoms that create molecule, number of molecules increases
        if (shared->atoms_in == 3){shared->mol_num++;shared->atoms_in = 0;}
        //with every atom that took part in creating molecule, we subtract it from remaining atoms
        if (atom == 'O'){shared->ox_num--;}
        if (atom == 'H'){shared->hyd_num--;}
        fflush(shared->file_op);
    }
    //not enough H
    else if( todo == 4){
        fprintf(shared->file_op, "%ld: %c %d: not enough H\n",++shared->rows_cnt, atom, id);
        fflush(shared->file_op);
    }
    //not enough O or H
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

    //only one oxygen can access this section at the time
    sem_wait(&shared->ox);

    //if there is not enough atoms, function prints out "not enough.."
    if (shared->hyd_num < 2 || shared->ox_num < 1){
        //allowing another oxygen get here
        sem_post(&shared->ox);

        //but before it prints, it must wait until all atoms are in queue
        if (shared->atoms_num_inq != shared->OX_n_HYD){
            sem_wait(&shared->wait_till_all_in_q);
        }
        print_to_file(shared,id,atom,4);
        exit(EXIT_SUCCESS);
    }


    //two hydrogen and one oxygen wait for each other here so none of them starts creating before they all are in queue
    sem_wait(&shared->begin);
    sem_wait(&shared->begin);
    sem_post(&shared->hyd);
    sem_post(&shared->hyd);


    //creating molecule
    print_to_file(shared, id,atom, 2);


    //here they wait for each other so they all print out creating before any of them prints "created"
    sem_wait(&shared->mutex2);
    sem_wait(&shared->mutex2);
    sem_post(&shared->mutex3);
    sem_post(&shared->mutex3);

    //sleeping and created
    rand_sleep(TB);
    print_to_file(shared, id, atom, 3);

    //waiting again to avoid deadlock
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

    //only two hydrogen can access this section at the time:
    sem_wait(&shared->mutex);

    //if there is not enough atoms, function prints out "not enough.."
    if (shared->hyd_num < 2 || shared->ox_num < 1){
        //but before it prints, it must wait until all atoms are in queue
        if (shared->atoms_num_inq != shared->OX_n_HYD){
            sem_wait(&shared->wait_till_all_in_q);
        }
        print_to_file(shared,id,atom,5);

        //allows another hydrogen access this section
        sem_post(&shared->mutex);
        exit(EXIT_SUCCESS);
    }

    //here two hydrogen wait for oxygen and reverse
    sem_post(&shared->begin);
    sem_wait(&shared->hyd);


    //here they create molecule
    print_to_file(shared, id,atom, 2);

    //they wait here for each other again so none of them prints "created.." before they all printed "creating.."
    sem_post(&shared->mutex2);
    sem_wait(&shared->mutex3);

    //creation time
    rand_sleep(TB);
    print_to_file(shared, id, atom, 3);//created

    //they again wait for each other here to avoid deadlock
    sem_post(&shared->mutex4);
    sem_wait(&shared->mutex5);

    sem_post(&shared->mutex);//another hydrogen can get going into critical section of this process

    exit(EXIT_SUCCESS);
}

void init_semaphores(shared_t *shared){
    sem_init(&(shared->out), 1, 1);
    sem_init(&(shared->mutex), 1, 2);
    sem_init(&(shared->mutex2), 1, 0);
    sem_init(&(shared->mutex3), 1, 0);
    sem_init(&(shared->mutex4), 1, 0);
    sem_init(&(shared->mutex5), 1, 0);
    sem_init(&(shared->begin), 1, 0);
    sem_init(&(shared->ox),1,1);
    sem_init(&(shared->hyd),1,0);
    sem_init(&(shared->wait_till_all_in_q),1,0);
    }

void destroy_semaphores(shared_t *shared){
    sem_destroy(&(shared->out));
    sem_destroy(&(shared->mutex));
    sem_destroy(&(shared->mutex2));
    sem_destroy(&(shared->mutex3));
    sem_destroy(&(shared->mutex4));
    sem_destroy(&(shared->mutex5));
    sem_destroy(&(shared->ox));
    sem_destroy(&(shared->hyd));
    sem_destroy(&(shared->begin));
    sem_destroy(&(shared->wait_till_all_in_q));
}
int main(int argc, char **argv){
    //checking parameters
    if (argc != ARG_NUM){
        fprintf(stderr,"[ERROR] invalid amount of arguments given\n");
        exit(EXIT_FAILURE);
    }

    int NO = atoi(argv[1]);
    int NH = atoi(argv[2]);
    int TI = atoi(argv[3]);
    int TB = atoi(argv[4]);
    //if given parameters are not in range <0,1000> program exits
    if (!(inRange(0,1000,TI) && inRange(0,1000,TB))){
        fprintf(stderr, "[ERROR] wrong range of given arguments\n");
        exit(EXIT_FAILURE);
    }

    //CHECKING WHETHER GIVEN ARGUMENTS ARE NUMBERS
    char* block;
    int ox = (int)strtol(argv[1], &block, 0);
    if (*block != '\0' || ox     < 0)
    {
        fprintf(stderr,"[ERROR] arguments must be numeric\n");
        exit(EXIT_FAILURE);
    }

    int hyd = (int)strtol(argv[2], &block, 0);
    if (*block != '\0' || hyd < 0)
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

    if (NO <= 0 || NH <= 0 || TI < 0 || TB < 0){
        fprintf(stderr, "[ERROR] NO and NH can not be negative nor zero values\n");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[4]) == 0 || strlen(argv[3]) == 0 || argv[2] == 0 || strlen(argv[1]) == 0 ){
        fprintf(stderr, "[ERROR] missing TI or TB\n");
        exit(EXIT_FAILURE);
    }


    //allocating the shared memory
    shared_t *shared;
    MMAP(shared);
    //initialization of all semaphores
    init_semaphores(shared);

    //declaring variables
    shared->file_op =  fopen("proj2.out", "w");
    shared->atoms_num_inq = 0;
    shared->atoms_in = 0;
    shared->mol_num = 1;

    int ox_num = atoi(argv[1]);
    int hyd_num = atoi(argv[2]);

    shared->ox_num = atoi(argv[1]);
    shared->hyd_num = atoi(argv[2]);

    shared->OX_n_HYD = atoi(argv[1]) + atoi(argv[2]);



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
            destroy_semaphores(shared);
            fclose(shared->file_op);
            UNMAP(shared);
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
            destroy_semaphores(shared);
            fclose(shared->file_op);
            UNMAP(shared);
            exit(EXIT_FAILURE);
        }
    }

    //destroying sems and freeing memory
    destroy_semaphores(shared);
    fclose(shared->file_op);
    UNMAP(shared);
    exit(0);
}
