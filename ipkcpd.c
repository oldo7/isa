#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

#define MAX_BUFFER 4000

int welcome_socket;
int comm_socket;
int server_socket;
fd_set current_sockets, ready_sockets;

//preskoci prave jednu medzeru
void skip_space(char* input, int* i, int* ERR){
    if(input[*i] == ' '){
        (*i)++;
    }
    else{
        *ERR = 1;
    }
}

//skontroluje ci po SOLVE nasleduje za prazdnymi znakmi zatvorka
int check_parenthesis_first(char* buffer){
    int i = 5;
    while(buffer[i] == ' '){
        i++;
    }
    if(buffer[i] == '('){
        return 1;
    }
    return 0;
}

int solve_expr(char* input, int* i, int* ERR){
    int morethan1op = 0;
    int x = 0;
    char operator;

    //ak to je cislo, vrati cislo
    if(input[*i] >= 48 && input[*i] <= 57){
        while(input[*i] >= 48 && input[*i] <= 57){
            x = input[*i] - '0' + 10 * x;
            (*i)++;
        }
        return x;
    }
    
    //ak to je zatvorka, rekurzivne vyriesi zatvorku a vrati vysledok
    if(input[*i] == '('){
        (*i)++;

        //zisti operator
        if(input[*i] == '+' || input[*i] == '-' || input[*i] == '*' || input[*i] == '/'){
            operator = input[*i];
            (*i)++;
        }else{
            *ERR = 1;
            return -1;
        }

        skip_space(input, i,ERR);

        //do x sa priradi vysledok prveho vyrazu
        x = solve_expr(input, i, ERR);

        //pokial nanarazi na koniec zatvorky, aplikuje operator na vsetky nasledujuce vyrazy, ktore vyriesi rekurzivne
        while(input[*i] != ')'){
            skip_space(input, i, ERR);
            morethan1op = 1;
            int result;
            switch(operator){
                case '+':
                    x += solve_expr(input, i, ERR);
                    break;
                case '*':
                    x *= solve_expr(input, i, ERR);
                    break;
                case '-':
                    x -= solve_expr(input, i, ERR);
                    break;
                case '/':
                    result = solve_expr(input, i, ERR);
                    if(result != 0){
                        x /= result;
                    }
                    else{
                        *ERR = 1;
                        break;
                    }
                    break;
            }

            //kontrola ci sa vo vyraze neobjavil neocakavany znak
            if(input[*i] != ' ' && !(input[*i] >= 48 && input[*i] <= 57) && input[*i] != '(' && input[*i] != ')'){
                *ERR = 1;
                return -1;
            }
        }
        if(!morethan1op){
            *ERR = 1;
        }
        (*i)++;
        return x;
    }

    //vo vyraze po preskoceni medzier nebola ani zatvorka ani cislo: chyba
    *ERR = 1;
    return -1;
}

void terminate_comm(){
    shutdown(server_socket, SHUT_RDWR);
    shutdown(welcome_socket, SHUT_RDWR);
    close(server_socket);
    close(welcome_socket);
    for(int j = 0; j<FD_SETSIZE; j++){
        if(FD_ISSET(j, &ready_sockets)){
            shutdown(j, SHUT_RDWR);
            close(j);
        }
    }
    exit(EXIT_SUCCESS);
}

void parse_tcp_input(int j, int (*helloflags)[FD_SETSIZE] , int (*byeflags)[FD_SETSIZE],char* recieve_buffer,char* send_buffer){
    //spracovanie vstupu
    char result[MAX_BUFFER] = "";
    int ERR = -1;
    if((*helloflags)[j] == 0){             //vyriesenie HELLO na zaciatku spojenia
        if(!strcmp(recieve_buffer, "HELLO\n")){
            (*helloflags)[j] = 1;
            strcpy(send_buffer, "HELLO\n");
        }else{
            ERR = 1;
        }
    }
    else{                           //ak hello uz bolo prijate, vyhodnoti vyraz
        if(strncmp(recieve_buffer, "SOLVE ", 6)){
            ERR = 1;
        }else{
            if(!check_parenthesis_first(recieve_buffer)){
                ERR = 1;
            }
            int i = 6;
            sprintf(result, "%d", solve_expr(recieve_buffer, &i, &ERR));
            if(result[0] == '-' || recieve_buffer[i] != '\n'){           //vysledok nemoze vyjst zaporny - ak vyjde, je to chyba, za zatvorkov musi nasledovat \n
                ERR = 1;
            }
            strcpy(send_buffer, "RESULT ");
            strcat(send_buffer, result);
            strcat(send_buffer, "\n");
        }               
    }
    if((!strcmp(recieve_buffer, "BYE")) || ERR == 1){               //ak vo vyraze bola chyba alebo bol prijaty retazec BYE, odosle sa BYE
        strcpy(send_buffer, "BYE\n");
        (*byeflags)[j] = 1;
    }
}

int create_udp_response(int status_code, char* message, char* send_buffer){
    send_buffer[0] = 1;
    send_buffer[1] = 'x';
    send_buffer[2] = strlen(message);
    strcat(send_buffer, message);
    send_buffer[1] = status_code;
    return strlen(message)+3;
}

int parse_udp_input(char* recieve_buffer,char* send_buffer){
    char send_message[127];
    int ERR = 0;
    int status_code = 0;
    if(recieve_buffer[0] != 0){
        strcpy(send_message, "Invalid opcode");
        ERR = 1;
    }
    if(recieve_buffer[2] != '('){
        strcpy(send_message, "Could not parse the message");
        ERR = 1;
    }
    
    if(ERR == 1){
        return create_udp_response(1,send_message, send_buffer);
    }
    int i = 2;
    sprintf(send_message, "%d", solve_expr(recieve_buffer, &i, &ERR));

    //kontrola ci sa spracovany vyraz skladal z ocakavaneho poctu znakov
    if((recieve_buffer[i] != 0 && recieve_buffer[i] != '\n') || send_message[0] == '-'){        //ihned za danym vyrazom musi byt '\0' alebo '\n'
        ERR = 1;
    }

    if(ERR != 1){
        return create_udp_response(0,send_message, send_buffer);
    }
    else{
        strcpy(send_message, "Could not parse the message");
        return create_udp_response(1,send_message, send_buffer);
    }
}

void printhelp(){
    printf("\nIPKCPD: Server pre IPK Calculator Protocol \n");
    printf("\n Pouzitie: ipkcpd -h <host> -p <port> -m <mode> \n \nKde <host> je IPv4 adresa na ktorej bude server pocuvat, <port> je port na ktorom bude server pocuvat a <mode> je bud \"tcp\" alebo \"udp\" \n");
    printf("\n Priklad pouzitia: \" ipkcpd -h 127.0.0.1 -p 2023 -m udp \" \n");
    printf("\n Server podla vybraneho modu pouzije bud binarnu (udp) alebo textovu (tcp) variantu protokolu. Aplikacia je vyvinuta pre Linux.\n");
}

int get_socket_tcp(){
    int family = AF_INET;
    int type = SOCK_STREAM;
    int welcome_socket = socket(family, type, 0);
    if (welcome_socket <= 0){
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }
    return welcome_socket;
}

int get_socket_udp(){
    int family = AF_INET;
    int type = SOCK_DGRAM;
    int server_socket = socket(family, type, 0);
    if (server_socket <= 0){
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }
    return server_socket;
}

//funkcia pre vytvorenie a naplnenie struktury sockaddr_in
struct sockaddr_in get_adress(char* host, int port){
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host %s\n", host);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    return server_addr;
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

    struct sockaddr_in server_addr = get_adress(host, port);
    struct sockaddr *address = (struct sockaddr *) &server_addr;

    signal(SIGINT, terminate_comm); 


    if(!strcmp(mode, "tcp")){
        //vytvorenie socketu
        int welcome_socket = get_socket_tcp();
        int enable = 1;
        setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        if (bind(welcome_socket, address, sizeof(server_addr)) < 0){
            perror("ERROR: bind");
            exit(EXIT_FAILURE);
        }

        if ((listen(welcome_socket, 1)) < 0){
            perror("ERROR: listen");
            exit(EXIT_FAILURE);
        }

        struct sockaddr *comm_addr = NULL;
        socklen_t comm_addr_size = 0;
        FD_ZERO(&current_sockets);
        FD_SET(welcome_socket, &current_sockets);

        //Pre kazdy mozny file descriptor sa uchovava informacia, ci uz bolo prijate HELLO pripadne BYE
        int helloflags[FD_SETSIZE];
        int byeflags[FD_SETSIZE];
        for(int j = 0 ; j<FD_SETSIZE; j++){
            helloflags[j] = 0;
            byeflags[j] = 0;
        }

        while(1){
            ready_sockets = current_sockets;
            if(select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0){
                perror("ERROR: select");
                exit(EXIT_FAILURE);
            }
            for(int j = 0; j<FD_SETSIZE; j++){
                if(FD_ISSET(j, &ready_sockets)){
                    if(j == welcome_socket){
                        //nove pripojenie
                        comm_socket = accept(welcome_socket, comm_addr, &comm_addr_size);
                        FD_SET(comm_socket, &current_sockets);
                    }
                    else{
                        if (j >= 0){
                            //ziskanie vstupu
                            char recieve_buffer[MAX_BUFFER] = "";
                            int bytes_rx = recv(j, recieve_buffer, MAX_BUFFER, 0);
                            if (bytes_rx <= 0) {
                                shutdown(j, SHUT_RDWR);
                                close(j);
                                FD_CLR(j, &current_sockets);
                                helloflags[j] = 0;
                                byeflags[j] = 0;
                                break;
                            }
                            
                            //spracovanie vstupu
                            char send_buffer[MAX_BUFFER] = "";
                            parse_tcp_input(j, &helloflags, &byeflags, recieve_buffer, send_buffer);
                            
                            //odoslanie vystupu a pripadne ukoncenie spojenia
                            int bytes_tx = send(j, send_buffer, strlen(send_buffer), 0);
                            if (bytes_tx <= 0 || byeflags[j] == 1) {
                                shutdown(j, SHUT_RDWR);
                                close(j);
                                helloflags[j] = 0;
                                byeflags[j] = 0;
                                FD_CLR(j, &current_sockets);
                                break;
                            }
                        }
                        else{
                            printf("chyba accept: %d", errno);
                        }
                    }
                }
            }
        }
    }

    else{
        server_socket = get_socket_udp();
        if (bind(server_socket, address, sizeof(server_addr)) < 0){
            perror("ERROR: bind");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in client_address;
        socklen_t address_size = sizeof(client_address);
        struct sockaddr *address = (struct sockaddr *) &client_address;

        while(1){
            //ziska vstup od klienta
            char recieve_buffer[MAX_BUFFER];
            int bytes_rx = recvfrom(server_socket, recieve_buffer, MAX_BUFFER, 0, address, &address_size);
            if (bytes_rx < 0) {
                perror("ERROR: read");
                continue;
            }
            
            //spracuje vstup
            char send_buffer[MAX_BUFFER] = "";
            int size = parse_udp_input(recieve_buffer, send_buffer);

            //odosle odpoved
            int bytes_tx = sendto(server_socket, send_buffer, size, 0, address, address_size);
            if (bytes_tx < 0) {
                perror("ERROR: sendto");
            }
        }
    }
     
}