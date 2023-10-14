#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

//vymazat
void print_udp_response(char* response){
    if(response[1] == 0){
        printf("OK:");
    }else if(response[1] == 1){
        printf("ERR:");
    }else{
        fprintf(stderr,"Prijaty neznamy status code");
    }
    for(int i = 3;i<response[2]+3;i++){
        printf("%c",response[i]);
    }
    printf("\n");
}
//vymazat

int get_socket_udp(){
    int family = AF_INET;
    int type = SOCK_DGRAM;
    int nsocket = socket(family, type, 0);
    if (nsocket <= 0){
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }
    return nsocket;
}

struct sockaddr_in get_adress(char* host, int port){
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host %s\n", host);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr.s_addr, server->h_addr, server->h_length);
    return server_address;
}

int main(int argc, char *argv[]){
    //argument parse
    int flagr = 0, flagx = 0, flag6 = 0, flags = 0, flagp = 0, flaga = 0, port = 53;
    char* dotaddr;      //dotazovana adresa
    char* host;         //server na ktory sa zasle dotaz
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
            dotaddr = argv[i];
        }
        else if(!strcmp(argv[i], "-p") && flagp == 0){
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
            host = argv[i];
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

    struct sockaddr_in server_address = get_adress(host, port);
    struct sockaddr *addr = (struct sockaddr *) &server_address;

    //upravit tak aby odoslany UDP packet bol DNS dotaz, ziadny while cyklus tam nebude
    size_t len = 0;
    int client_socket = get_socket_udp();
        while(1){
            //ziska uzivatelsky vstup a posle ho
            char *input = NULL;
            getline(&input, &len, stdin);
            int flags = 0;
            char dest[strlen(input)+2];
            dest[0]= 'x';
            dest[1]= strlen(input);
            dest[2]= '\0';
            strcat(dest,input);
            dest[0]= '\0';
            
            int bytes_tx = sendto(client_socket, dest, strlen(input)+2,flags, addr, sizeof(server_address));
            if (bytes_tx < 0){
                perror("ERROR: sendto");
            }
            free(input);
            
            //ziska odpoved a vytiskne ju
            socklen_t address_size = sizeof(server_address);
            char response[4000] = "";
            int bytes_rx = recvfrom(client_socket, response, 4000,flags, addr, &address_size);
            if (bytes_rx < 0){
                perror("ERROR: recvfrom");
            }
            print_udp_response(response);
        }
    //upravit tak aby odoslany UDP packet bol DNS dotaz
}