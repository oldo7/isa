#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#define MAX_RESPONSE 4000

int client_socket;      //globalna premenna, pretoze v pripade SIGINT je socket nutne uzavriet z funkcie volanej funkciou signal(), a teda do nej nieje mozne priradit parameter

int get_socket_tcp(){
    int family = AF_INET;
    int type = SOCK_STREAM;
    int nsocket = socket(family, type, 0);
    if (nsocket <= 0){
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }
    return nsocket;
}

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

//funkcia pre vytvorenie a naplnenie struktury sockaddr_in
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

void terminate(){
    shutdown(client_socket, SHUT_RD);
    shutdown(client_socket, SHUT_WR);
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    exit(EXIT_SUCCESS);
}

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

void printhelp(){
    printf("\n Pouzitie: ipkcpc -h <host> -p <port> -m <mode> \n \nKde <host> je IPv4 adresa servera, <port> je server port a <mode> je bud \"tcp\" alebo \"udp\" \n");
    printf("Priklad : ipkcpc -h 1.2.3.4 -p 2023 -m udp \n \n Podla vybraneho modu sa pouzije protokol (binarny pre udp, textovy pre tcp). Zo standardneho vstupu sa precita prikaz,");
    printf("jeden na riadok, odosle sa specifikovanemu serveru, a prijata odpoved sa vypise na standardny vystup.\n Priklad vstupu (binarny): \n \t(+ 1 2) \n Priklad vstupu (textovy): \n \tHELLO \n \tSOLVE (+ 1 2)\n \tBYE\n");
}

int main(int argc, char *argv[]){
    if(!strcmp(argv[1],"--help")){
        printhelp();
        exit(EXIT_SUCCESS);
    }
    if(argc != 7){
        fprintf(stderr,"Nespravne mnozstvo argumentov.");
        printhelp();
        exit(EXIT_FAILURE);
    }
    if(strcmp(argv[1],"-h") || strcmp(argv[3],"-p") || strcmp(argv[5],"-m") || (strcmp(argv[6],"tcp") && strcmp(argv[6],"udp"))){
        fprintf(stderr,"Nespravne argumenty.");
        printhelp();
        exit(EXIT_FAILURE);
    }
    char* host = argv[2];
    int port = atoi(argv[4]);
    char* mode = argv[6];


    struct sockaddr_in server_address = get_adress(host, port);
    struct sockaddr *addr = (struct sockaddr *) &server_address;

    signal(SIGINT, terminate); 
    size_t len = 0;

    if(!strcmp(mode, "tcp")){
        client_socket = get_socket_tcp();
        if (connect(client_socket, addr, sizeof(server_address)) != 0){
            perror("ERROR: connect");
            exit(EXIT_FAILURE);
        }

        int byeflag = 0;

        while(1){
            //ziska uzivatelsky vstup a posle ho
            char *input = NULL;
            getline(&input, &len, stdin);
            int bytes_tx = send(client_socket, input, strlen(input), 0);
            if (bytes_tx < 0) {
                perror("ERROR: send");
            }
            if(!strcmp("BYE\n",input)){
                byeflag = 1;
            }
            free(input);

            //ziska odpoved a vytiskne ju
            char response[MAX_RESPONSE] = "";
            int bytes_rx = recv(client_socket, response, 4000, 0);
            if (bytes_rx < 0) {
                perror("ERROR: recv");
            }
            printf("%s",response);
            
            if(byeflag){
                terminate();
            }
        }
    }
    else{   //mode = udp
        client_socket = get_socket_udp();
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
            char response[MAX_RESPONSE] = "";
            int bytes_rx = recvfrom(client_socket, response, MAX_RESPONSE,flags, addr, &address_size);
            if (bytes_rx < 0){
                perror("ERROR: recvfrom");
            }
            print_udp_response(response);
        }
    }  
}

