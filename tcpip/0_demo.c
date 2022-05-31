#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

void error_handling(char *message);

int main(int argc,char *argv[]){
    
    if(argc!=2){
        printf("Usage: %s <ip> \n",argv[0]);
        exit(1);
    }
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}