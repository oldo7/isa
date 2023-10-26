//Autor: Oliver Nemcek, xnemce08

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[]){
    int succ = 0; 
    int fail = 0; 
    int status;
    //poziadavok na server ktory neodpovie
    status = system("./dns -s localhost 2001:4860:4802:34:0:0:0:a -x > nul");
    if(status/256 == 3){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s google.com seznam.cz > nul");
    if(status/256 == 3){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s seznam.cz seznam.cz -r > nul");
    if(status/256 == 3){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s seznam.cz seznam.cz -r -x > nul");
    if(status/256 == 3){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 91.235.53.86 google.com -r -x > nul");
    if(status/256 == 3){
        succ++;
    }else{
        fail++;
    }

    //poziadavok na server ktory neexistuje
    status = system("./dns -s 123.0.q google.com -r -x > nul");
    if(status/256 == 5){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s -r -x -x > nul");
    if(status/256 == 5){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s -r -x vut.cz > nul");
    if(status/256 == 5){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s lolecq google.com -r -x > nul");
    if(status/256 == 5){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 1.1.1.1.1 google.com -r -x > nul");
    if(status/256 == 5){
        succ++;
    }else{
        fail++;
    }

    //poziadavok s domenovym menom ktore neexistuje
    status = system("./dns -s 147.229.190.143 1.1.1.1.1 -r -x > nul");
    if(status/256 == 1){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 8.8.8.8 -x seznam.cz > nul");                     //je pouzity priznak -x ale je zadane domenove meno
    if(status/256 == 1){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 147.229.190.143 lolecq -r > nul");
    if(status/256 == 1){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s arekol.kn.vutbr.cz 123.0.q -r -x > nul");
    if(status/256 == 1){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s arekol.kn.vutbr.cz vutabc.com -r > nul");
    if(status/256 == 1){
        succ++;
    }else{
        fail++;
    }

    //chybne zadane parametre
    status = system("./dns -s 8.8.8.8 vut.cz -r -6 -x > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns 8.8.8.8 vut.cz -r -6 -x > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 8.8.8.8 -r -x > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns abcde vut.cz > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s 651 615 dsad > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s -r -x -x -x > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -s > nul");
    if(status/256 == 4){
        succ++;
    }else{
        fail++;
    }

    //spravne hodnoty
    status = system("./dns -s 8.8.8.8 seznam.cz > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns seznam.cz -s 8.8.8.8 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns seznam.cz -s 8.8.8.8 -r > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -r seznam.cz -s 8.8.8.8 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -r -6 seznam.cz -s 8.8.8.8 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -6 -r seznam.cz -s 8.8.8.8 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns 91.235.53.86 -s 147.229.190.143 -x > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -x 91.235.53.86 -s 147.229.190.143 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -x 91.235.53.86 -s arekol.kn.vutbr.cz > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -6 google.com -s 147.229.190.143 > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -6 google.com -s 147.229.190.143 -r > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    status = system("./dns -6 google.com -s arekol.kn.vutbr.cz -r > nul");
    if(status/256 == 0){
        succ++;
    }else{
        fail++;
    }

    printf("Pocet testov ktore presli: %d / %d\n", succ, (succ+fail));
}