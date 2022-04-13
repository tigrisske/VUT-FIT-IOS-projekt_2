#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARG_NUM 5


bool inRange(int min, int max, int arg){
    if (arg >= min && arg <= max){
        return true;
    }
    return false;
}


int main(int argc, char **argv){
    int cislo = 34;
    if (isdigit(cislo)){
        return 0;
    }
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
    return 0;
}
