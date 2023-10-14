#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>



int main(int argc, char *argv[]){
    //argument parse
    int flagr = 0, flagx = 0, flag6 = 0, flags = 0, flagp = 1, flaga = 0, port = 53;
    char* ser;
    char* addr;
    for(int i = 1; i < argc; i++){
        if(!strcmp(argv[i], "-r") && flagr == 0){
            flagr = 1;
        }
        else if(!strcmp(argv[i], "-x") && flagx == 0){
            flagx = 1;
        }
        else if(!strcmp(argv[i], "-6") && flag6 == 0){
            flag6 = 1;
        }
        else if(!strcmp(argv[i], "-s") && flags == 0){
            flags = 1;
            if(argc == i+1){
                fprintf(stderr, "Nespravne zadane argumenty");
                exit(EXIT_FAILURE);
            }
            i++;
            ser = argv[i];
        }
        else if(!strcmp(argv[i], "-p") && flags == 0){
            flagp = 1;
            if(argc == i+1){
                fprintf(stderr, "Nespravne zadane argumenty");
                exit(EXIT_FAILURE);
            }
            i++;
            port = atoi(argv[i]);
        }
        else if(flaga == 0){
            flaga = 1;
            addr = argv[i];
        }
        else{
            fprintf(stderr, "Nespravne zadane argumenty");
            exit(EXIT_FAILURE);
        }
    }
    if(flaga == 0 || flags == 0){
        fprintf(stderr, "Nespravne zadane argumenty: chyba adresa alebo server");
        exit(EXIT_FAILURE);
    }

    //kontrola formatu parametrov
    if(port == 0 || port > 65535){
        fprintf(stderr, "Nespravne zadane argumenty: nevalidne cislo portu");
        exit(EXIT_FAILURE);
    }

    //printf("flagr: %d \n flagx: %d \n flag6: %d \n server: %s \n port: %d \n adresa: %s \n", flagr, flagx, flag6, ser, port, addr);
    //test
}