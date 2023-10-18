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
int make_header(unsigned char *header, int rd){
    header[0] = rand()%256;
    header[1] = rand()%256;
    if(rd){                         //recursion desired bit
        header[2] = 0x01;
    }else{
        header[2] = 0x00;
    }
    header[3] = 0x00;

    header[4] = 0x00;
    header[5] = 0x01;           //qdcount

    header[6] = 0x00;
    header[7] = 0x00;           //ancount

    header[8] = 0x00;
    header[9] = 0x00;           //nscount

    header[10] = 0x00;
    header[11] = 0x00;          //arcount

    return header[0]*256 + header[1]; //vrati hodnotu ID na neskorsiu kontrolu
}

void make_body(unsigned char *body, unsigned char *dotaddr, int qtype){
    int i = 0;
    int j = 0;
    //TODO: ostatne typy request (AAAA a reverse)
    switch (qtype)
    {
    case 1:                     // A
        while(dotaddr[j] != '\0'){
            j = i;
            while(dotaddr[j] != '.' && dotaddr[j] != '\0'){           //zisti index najblizsej bodky
                j++;
            }
            int numofchars = j-i;                           //pocet nasledujucich znakov
            body[i] = numofchars;                           //prvy byte labelu je pocet znakov ktore nasleduju
            i++;
            for(int k = 0; k<numofchars; k++){              //vlozi dane znaky do retazca
                body[i] = dotaddr[i-1];
                i++;
            }
        }
        body[i] = '\0';                                     //byte na konci sekvencii labelov

        body[i+1] = 0;
        body[i+2] = 1;                                      //QTYPE = 1 (A)
        
        body[i+3] = 0;
        body[i+4] = 1;                                      //QCLASS = 1 (Internet address)
        return;
    
    default:
        break;
    }
}

//naplni retazec query z retazcov header a body
void make_query(unsigned char *query, unsigned char *header, unsigned char *body, int qlength){
    int i = 0;
    for(i; i<12; i++){
        query[i] = header[i];
    }
    while(i<qlength){
        query[i] = body[i-12];
        i++;
    }
    return;
}

//funkcia vytiskne polia NAME, TYPE a CLASS. vracia type.
int printquestion(int* i, unsigned char* response, int onlyname){
    int skipname = 0;                        //flag pre urcenie, ci uz NAME bolo precitane
    if((response[*i] & 0xc0) == 0xc0){       //ak prve dva bity su jednotky, jedna sa o skomprimovanu adresu (odkaz na predosly vyskyt adresy)
        int tempi = *i;
        tempi = (response[tempi] & 0x3f) * 256 + response[tempi+1];             //hodnota offsetu na ktorom sa nachadza odkazovany nazov
        printquestion(&tempi, response, 1);                                        //vola sa funkcia printquestion s ukazatelom na danom offsete
        *i += 2;                            //ukazatel sa inkrementuje o velkost odkazu (2 byte)
        skipname = 1;
    }

    if(skipname==0){
        while(response[*i] != 0){
            if(response[*i] == 0xc0){
                printquestion(i, response, onlyname);          //ak jedna z "bodiek" (labelov) je 0xc0, jedna sa o skomprimovany nazov. zvysok nazvu sa dopise opatovnym zavolanim tejto funkcie.
                (*i)--;   //algoritmus obsluhy skrateneho mena automaticky inkrementuje ukazatel i o velkost odkazu (2 byte). sucastou cyklu vypisania mena je vsak inkrementacia ukazatela i na konci. teda by sme boli o jeden znak vpred.
                break;
            }
            int nextdot = *i+response[*i];
            for(*i;(*i)<=nextdot;(*i)++){
                printf("%c",response[*i]);
            }
            printf(".");
        }
        (*i)++;
    }
    

    if(onlyname == 1){
        return -1;
    }

    int qtype = response[*i]*256 + response[*i+1];
    (*i)+=2;
    int qclass = response[*i]*256 + response[*i+1];
    (*i)+=2;       //kurzor je na prvom znaku za otazkov

    switch (qtype)
    {
    case 1:
        printf(",A");
        break;
    case 28:
        printf(",AAAA");
        break;
    case 5:
        printf(",CNAME");
        break;
    case 12:
        printf(",PTR");
        break;
    default:
        printf("%d", qtype);
        break;
    }

    switch (qclass)
    {
    case 1:
        printf(",IN");
        break;
    default:
        printf(",%d", qclass);
        break;
    }
    return qtype;
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
    
    //vypisanie statistik:
    if(aa){
        printf("Authoritative: Yes, ");
    }else{
        printf("Authoritative: No, ");
    }

    if(rd && ra){
        printf("Recursive: Yes, ");
    }else{
        printf("Recursive: No, ");
    }

    if(tc){
        printf("Truncated: Yes ");
    }else{
        printf("Truncated: No ");
    }

    //vypisanie question sekcie
    printf("\n Question section (%d) \n ", qdcount);
    int i = 12;
    printquestion(&i, response, 0);
    
    //vypisanie answer sekcie
    printf("\n Answer section (%d) \n ", ancount);
    for(int j = 0; j < ancount; j++){
        int qtype = printquestion(&i, response, 0);         //vypis name, type a class
        int TTL = (response[i] << 24) + (response[i+1] << 16) + (response[i+2] << 8) + (response[i+3]);     //vypis ttl
        printf(",%d,", TTL);
        i+=4;
        int rdlength = (response[i] << 8) + response[i+1];
        i+=2;

        //todo: do switchu pridat ostatne typy, tie co nepodporujeme skipnut podla ich velkosti
        switch (qtype)                      //data sa vypisuju podla toho, o aky typ zaznamu ide
        {
        case 5:
            printquestion(&i, response, 1);             //ak je zaznam CNAME, pouzije sa funkcia printquestion na vypisanie nazvu
            break;
        case 1:
            printf("%d.%d.%d.%d", response[i], response[i+1], response[i+2], response[i+3]);        //ak je zaznam A, vypise sa IP adresa
            i += 4;
            break;
        default:
            break;
        }
        printf("\n");
    }

    //TODO: authority a additional sekcie

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

    if(flagx == 1 && flag6 == 1){
        fprintf(stderr, "Kombinacia parametrov -6 a -x nieje povolena");
        exit(EXIT_FAILURE);
    }
    int qtype = 1;             //qtype dotazu. default: 1 (A)
    if(flag6){
        qtype = 28;             // AAAA
    }else if(flagx){
        qtype = 12;             // PTR
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

    //vytvori header
    unsigned char header[12]; //header ma pevnu dlzku 12 bytov
    int ID = make_header(header, flagr);
    //vytvori body
    unsigned char body[strlen(dotaddr) + 6];         //velkost tela je strlen (kazda bodka bude nahradena bytom velkosti dalsieho labelu) + 2 (zaciatocny byte labelu a \0 na konci) + QTYPE (2) a QCLASS (2)
    make_body(body, dotaddr, qtype);
    
    //TODO MAIN: mame header a body, spojit ich a odoslat to
    int qlength = 12 + strlen(dotaddr) + 6;
    unsigned char query[qlength];
    make_query(query, header, body, qlength);


    //odosle vytvoreny DNS dotaz
    int bytes_tx = sendto(client_socket, query, qlength,0 , addr, sizeof(server_address));
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