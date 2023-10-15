#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MAX_RESPONSE 512

//funkcia ktora ako parametre vezme pole header a hodnoty jednotlivych flagov, a pole naplni spravnymi hodnotami hlavicky. vrati naplnenu hlavicku.
int make_header(unsigned char *header){
    header[0] = rand()%256;
    header[1] = rand()%256;
    header[2] = 0x01;
    header[3] = 0x00;
    header[4] = 0x00;
    header[5] = 0x01;
    header[6] = 0x00;
    header[7] = 0x00;
    header[8] = 0x00;
    header[9] = 0x00;
    header[10] = 0x00;
    header[11] = 0x00;
    header[12] = 0x03;
    header[13] = 0x77;
    header[14] = 0x77;
    header[15] = 0x77;
    header[16] = 0x0c;
    header[17] = 0x6e;
    header[18] = 0x6f;
    header[19] = 0x72;
    header[20] = 0x74;
    header[21] = 0x68;
    header[22] = 0x65;
    header[23] = 0x61;
    header[24] = 0x73;
    header[25] = 0x74;
    header[26] = 0x65;
    header[27] = 0x72;
    header[28] = 0x6e;
    header[29] = 0x03;
    header[30] = 0x65;
    header[31] = 0x64;
    header[32] = 0x75;
    header[33] = 0x00;
    header[34] = 0x00;
    header[35] = 0x01;
    header[36] = 0x00;
    header[37] = 0x01;

    return header[0]*256 + header[1]; //vrati hodnotu ID na neskorsiu kontrolu
}

void print_dns_response(int ID, unsigned char *response){
    //kontrola ci prijate id je rovnake ako odoslane
    if(ID != response[0]*256 + response[1]){
        fprintf(stderr, "prijata odpoved nebola sparovana s odoslanym dotazom");
        exit(EXIT_FAILURE);
    }

    //kontrola flagu QR
    unsigned char qr = response[2] & 0x80;
    if(qr == 0){
        fprintf(stderr, "prijata odpoved ma nespravnu hodnotu QR");
        exit(EXIT_FAILURE);
    }

    //ak flag == 0 tak hodnota bude nulova, inak bude nenulova
    unsigned char aa = response[2] & 0x64;  
    unsigned char tc = response[2] & 0xA;
    unsigned char rd = response[2] & 0x1;
    unsigned char ra = response[3] & 0x80;
    unsigned char rcode = response[3] & 0xF;
    int qdcount = response[4] * 256 + response[5];
    int ancount = response[6] * 256 + response[7];
    int nscount = response[8] * 256 + response[9];
    int arcount = response[10] * 256 + response[11];

    switch (rcode)
    {
    case 1:
        printf("Response code 1 : Server nedokazal interpretovat poziadavok");
        exit(EXIT_SUCCESS);
        break;
    case 2:
        printf("Response code 2 : Chyba DNS servera");
        exit(EXIT_SUCCESS);
        break;
    case 3:
        printf("Response code 3 : Domenove meno v poziadavku neexistuje");
        exit(EXIT_SUCCESS);
        break;
    case 4:
        printf("Response code 4 : Server nepodporuje dany typ poziadavku");
        exit(EXIT_SUCCESS);
        break;
    case 5:
        printf("Response code 5 : Server odmietol vykonat pozadovanu operaciu");
        exit(EXIT_SUCCESS);
        break;
    default:
        break;
    }
    // printf(" aa = %x \n tc = %x \n rd = %x \n ra = %x \n rcode = %x \n qdcount = %d \n ancount = %d \n nscount = %d \n arcount = %d \n ", aa, tc, rd, ra, rcode, qdcount, ancount, nscount, arcount);

    //vypisanie jednotlivych sekcii

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
            host = argv[i];
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
            dotaddr = argv[i];
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

    // printf("flagr: %d \n flagx: %d \n flag6: %d \n server na ktory sa dotaz posiela: %s \n port: %d \n dotazovana adresa: %s \n", flagr, flagx, flag6, host, port, dotaddr);
    srand(time(NULL));

    struct sockaddr_in server_address = get_adress(host, port);
    struct sockaddr *addr = (struct sockaddr *) &server_address;

    int client_socket = get_socket_udp();

    //vytvorit header
    char header[38]; //header ma pevnu dlzku 12 bytov
    int ID = make_header(header);
    //make_body
    //join header and body

    //odosle vytvoreny DNS dotaz
    int bytes_tx = sendto(client_socket, header, 38,0 , addr, sizeof(server_address));
    if (bytes_tx < 0){
        perror("ERROR: sendto");
    }
    
    //ziska odpoved a vytiskne ju
    socklen_t address_size = sizeof(server_address);
    unsigned char response[MAX_RESPONSE] = "";

    int bytes_rx = recvfrom(client_socket, response, 65000,0, addr, &address_size);
    if (bytes_rx < 0){
       perror("ERROR: recvfrom");
    }

    print_dns_response(ID, response);
}