//Autor: Oliver Nemcek, xnemce08

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netdb.h>
#include <time.h>

#define MAX_RESPONSE 512
#define ERR_RESPONSE_CODE 1
#define ERR_RESPONSE_VALUES 2
#define ERR_NO_RESPONSE 3
#define ERR_PARAM 4
#define ERR_INTERNAL 5

void printhelp(){
    printf("\nPouzitie: dns [-r] [-x] [-6] -s server [-p port] adresa \n \n -r: Pozadovana rekurzia (Recursion Desired = 1), inak bez rekurzie. \n-x: Reverzny dotaz namiesto priameho. \n-6: Dotaz typu AAAA namiesto predvoleneho A. \n-s: IP adresa alebo domenove meno serveru, kam se ma zaslat dotaz. \n");
    printf("-p port: Cislo portu, na ktory sa ma poslat dotaz, predvolene 53. \nadresa: Dotazovana adresa. \n \nProgram odosle DNS dotaz na zadany server, a na standardny vystup vypise odpoved.\n");
    exit(ERR_PARAM);
}

void concat(char *result, char *string1, char *string2, int s1length, int s2length){
    int i = 0;
    for(i; i<s1length; i++){
        result[i] = string1[i];
    }
    while(i<s2length + s1length){
        result[i] = string2[i-s1length];
        i++;
    }
    return;
}

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
    switch (qtype)
    {
    case 28:                    //AAAA
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
        body[i+2] = qtype;                                  //QTYPE

        body[i+3] = 0;
        body[i+4] = 1;                                      //QCLASS = 1 (Internet address)
        return;
    case 12:                    //PTR
        j = strlen(dotaddr)-1;
        while(j >= 0){                                      //spravne doplni pocet znakov do labelov a vymeni poradie jednotlivych oktetov v ip adrese
            int temp = j;
            while(dotaddr[j] != '.' && j>= 0){
                j--;
            }
            int numofchars = temp - j;
            temp = j;
            j++;
            body[i] = numofchars;
            i++;
            for(int l = 0; l < numofchars; l++){
                body[i] = dotaddr[j];
                j++;
                i++; 
            }
            j = temp - 1;
        }   

        concat(body, body, "\x07in-addr\x04\x61rpa\0",i,14);        //za ip adresu vlozi staticky retazec
        i+=13;

        body[i+1] = 0;
        body[i+2] = qtype;                                  //QTYPE

        body[i+3] = 0;
        body[i+4] = 1;                                      //QCLASS = 1 (Internet address)
        break;
    case 1001:                                              //PTR(ipv6)
        qtype = 12;
        int zeros = -5;
        int coloncount = 0;                      //kolko : je v adrese (okrem ::)
        int zeroscount = 0;                     //pocet nul medzi ::
        for(int z = 0; z<strlen(dotaddr); z++){
            if(dotaddr[z] == ':'){
                if(dotaddr[z+1] == ':'){
                    zeros = z;
                    z+=2;
                }else{
                    coloncount++;
                }   
            }
        }
        if(zeros == 0 || zeros ==strlen(dotaddr) - 2){              //ak je :: na zaciatku alebo na konci
            zeroscount = 8-(coloncount+1);
        }else{
            zeroscount = 8-(coloncount+2);
        }

        j = strlen(dotaddr) - 1;
        while(j >= 0){
            int temp = j;
            while(dotaddr[j] != ':' && j>= 0){
                j--;
            }
            int numofchars = temp-j;
            if(j == zeros){                           //narazili sme na lavu stranu ::
                for(int z = 0; z<zeroscount; z++){      //pre kazdy zeroscount sa pridaju 4 nuly (aj s ich repektivnymi labelami)
                    for(int a = 0; a<4; a++){
                        body[2*i] = 1;
                        body[2*i + 1] = '0';
                        i++;
                    }
                }
                j--;
                continue;
            }
            for(int z=0; z<4; z++){                         //naplni 4 policka
                if(numofchars>0){
                    body[2*i] = 1;
                    body[2*i + 1] = dotaddr[temp];
                    temp--;
                    numofchars--;
                    i++;
                }
                else{
                    body[2*i] = 1;
                    body[2*i + 1] = '0';
                    i++;
                }
            }
            j--;                //j sa posunie z : na dalsiu stvoricu
            if(j == -1){        //prvym znakom bola : -> treba pridat 4 nuly na koniec
                for(int z=0; z<4; z++){                         //naplni 4 policka
                    body[2*i] = 1;
                    body[2*i + 1] = '0';
                    i++;
                }   
            }
        }
        i*=2;
        concat(body, body, "\x03ip6\x04\x61rpa\0",i,10);
        i+=9;

        body[i+1] = 0;
        body[i+2] = qtype;                                  //QTYPE

        body[i+3] = 0;
        body[i+4] = 1;                                      //QCLASS = 1 (Internet address)
        break;
    default:
        break;
    }
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
    case 2:
        printf(",NS");
        break;
    case 6:
        printf(",SOA");
        break;
    default:
        printf(",%d", qtype);
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
        fprintf(stderr, "Chyba: Nebola prijata ziadna odpoved od servera. Uistite sa, ze zadany server je DNS serverom. \n");
        exit(ERR_NO_RESPONSE);
    }

    //kontrola flagu QR
    unsigned char qr = response[2] & 0x80;
    if(qr == 0){
        fprintf(stderr, "prijata odpoved ma nespravnu hodnotu QR \n");
        exit(ERR_RESPONSE_VALUES);
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
    case 0:
        break;
    case 1:
        printf("Response code 1 : Server nedokazal interpretovat poziadavok\n");
        exit(ERR_RESPONSE_CODE);
        break;
    case 2:
        printf("Response code 2 : Chyba DNS servera");
        exit(ERR_RESPONSE_CODE);
        break;
    case 3:
        printf("Response code 3 : Domenove meno / adresa v poziadavku neexistuje\n");
        exit(ERR_RESPONSE_CODE);
        break;
    case 4:
        printf("Response code 4 : Server nepodporuje dany typ poziadavku\n");
        exit(ERR_RESPONSE_CODE);
        break;
    case 5:
        printf("Response code 5 : Server odmietol vykonat pozadovanu operaciu\n");
        exit(ERR_RESPONSE_CODE);
        break;
    default:
        printf("Response code %d \n", rcode);
        exit(ERR_RESPONSE_CODE);
    }
    
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
    
    //vypisanie answer, authority a additional sekcii
    for(int section = 0; section < 3; section++){
        int numofiterations = 0;                            //pocet poloziek v danej sekcii
        if(section == 0){
            printf("\n Answer section (%d) \n ", ancount);
            numofiterations = ancount;
        }else if(section == 1){
            printf("Authority section (%d) \n ", nscount);
            numofiterations = nscount;
        }else{
            printf("Additional section (%d) \n ", arcount);
            numofiterations = arcount;
        }
        for(int j = 0; j < numofiterations; j++){               //spracovavanie jednotlivych zaznamov
            int qtype = printquestion(&i, response, 0);         //vypis name, type a class
            int TTL = (response[i] << 24) + (response[i+1] << 16) + (response[i+2] << 8) + (response[i+3]);     //vypis ttl
            printf(",%d,", TTL);
            i+=4;
            int rdlength = (response[i] << 8) + response[i+1];
            i+=2;
            switch (qtype)                      //vypis dat: data sa vypisuju podla toho, o aky typ zaznamu ide
            {
            case 1:                                 //ak je zaznam A, vypise sa IP adresa
                printf("%d.%d.%d.%d", response[i], response[i+1], response[i+2], response[i+3]);
                i += 4;
                break;
            case 28:                                //AAAA, vypisa sa ipv6 adresa
                for(int k = 0; k<7;k++){
                    printf("%x:",(response[i] << 8) + response[i+1]);
                    i+=2;
                }
                printf("%x",(response[i] << 8) + response[i+1]);
                i+=2;
                break;
            case 12:
            case 5:
            case 2:                                 //ak je zaznam PTR, NS, alebo CNAME pouzije sa funkcia printquestion na vypisanie nazvu
                printquestion(&i, response, 1);
                break;
            default:
                printf("[Typ zaznamu nepodporovany]");
                i+= rdlength;
                break;
            }
            printf("\n");
        }
    }

}

int get_socket_udp(){
    int family = AF_INET;
    int type = SOCK_DGRAM;
    int nsocket = socket(family, type, 0);
    if (nsocket <= 0){
        perror("ERROR: socket\n");
        exit(ERR_INTERNAL);
    }

    struct timeval tv;                                                  //timeout, zdroj: https://stackoverflow.com/questions/13547721/udp-socket-set-timeout
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(nsocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Socket error\n");
        exit(ERR_INTERNAL);
    }

    return nsocket;
}

struct sockaddr_in get_adress(char* host, int port){
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Chyba: zadany DNS server neexistuje: %s\n", host);
        exit(ERR_INTERNAL);
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
                fprintf(stderr, "Chyba: Nespravne zadane argumenty\n");
                printhelp();
            }
            i++;
            host = argv[i];
        }
        else if(!strcmp(argv[i], "-p") && flagp == 0){
            flagp = 1;
            if(argc == i+1){
                fprintf(stderr, "Chyba: Nespravne zadane argumenty\n");
                printhelp();
            }
            i++;
            port = atoi(argv[i]);
        }
        else if(flaga == 0){
            flaga = 1;
            dotaddr = argv[i];
        }
        else{
            fprintf(stderr, "Chyba: Nespravne zadane argumenty\n");
            printhelp();
        }
    }
    if(flaga == 0 || flags == 0){
        fprintf(stderr, "Chyba: Nespravne zadane argumenty: chyba adresa alebo server\n");
        printhelp();
    }

    if(flagx == 1 && flag6 == 1){
        fprintf(stderr, "Kombinacia parametrov -6 a -x nieje povolena\n");
        printhelp();
    }
    int qtype = 1;             //qtype dotazu. default: 1 (A)
    if(flag6){
        qtype = 28;             // AAAA
    }else if(flagx){
        qtype = 12;             // PTR
    }

    //kontrola formatu parametrov
    if(port == 0 || port > 65535){
        fprintf(stderr, "Chyba: Nespravne zadane argumenty: nevalidne cislo portu\n");
        printhelp();
    }

    srand(time(NULL));

    struct sockaddr_in server_address = get_adress(host, port);
    struct sockaddr *addr = (struct sockaddr *) &server_address;

    int client_socket = get_socket_udp();

    //vytvori header
    unsigned char header[12]; //header ma pevnu dlzku 12 bytov
    int ID = make_header(header, flagr);
    //vytvori body
    int bodysize = strlen(dotaddr) + 6;             //velkost tela je strlen (kazda bodka bude nahradena bytom velkosti dalsieho labelu) + 2 (zaciatocny byte labelu a \0 na konci) + QTYPE (2) a QCLASS (2)
    if(qtype == 12){
        for(int i = 0; i<strlen(dotaddr); i++){
            if(dotaddr[i] == '.'){
                bodysize += 13;                             //velkost tela pri reverznom ipv4 dotaze je vacsia o konstantny vyraz .in-addr.arpa ktory sa konkatenuje na koniec
                break;
            }else if(dotaddr[i] == ':'){
                bodysize = 78;                             //velkost tela pri reverznom ipv6 ma pevne danu dlzku 78
                qtype = 1001;                               //interne sa nastavi type 1001, ktory odlisuje PTR ipv4 od ipv6
                break;
            }
            if(i == strlen(dotaddr)){
                fprintf(stderr, "Nevalidna kombinacia adresy a parametru -x\n");
                printhelp();
            }
        }
        
    }
    unsigned char body[bodysize];
    make_body(body, dotaddr, qtype);
    
    int qlength = 12 + bodysize;
    unsigned char query[qlength];
    concat(query, header, body, 12, bodysize);

    //odosle vytvoreny DNS dotaz
    int bytes_tx = sendto(client_socket, query, qlength,0 , addr, sizeof(server_address));
    if (bytes_tx < 0){
        perror("ERROR: sendto\n");
        exit(ERR_INTERNAL);
    }
    
    //ziska odpoved a vytiskne ju
    socklen_t address_size = sizeof(server_address);
    unsigned char response[MAX_RESPONSE] = "";

    int bytes_rx = recvfrom(client_socket, response, MAX_RESPONSE,0, addr, &address_size);
    if (bytes_rx < 0){
       perror("Chyba: Nebola prijata ziadna odpoved od servera. Uistite sa, ze zadany server je DNS serverom. \n");
       exit(ERR_INTERNAL);
    }

    print_dns_response(ID, response);
}