#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

//根据域名得到ip

#define BUF_SIZE 100

void error_handling(char *message);

int main(int argc,char *argv[]){

    struct hostent *host;

    if(argc!=2){
        printf("Usage: %s <addr> \n",argv[0]);
        exit(1);
    }

    host=gethostbyname(argv[1]);
    if(!host){
        error_handling("gethost... error");
    } 
    //官方域名 h_name
    printf("Official name: %s \n",host->h_name);
    //别名 h_aliased[i]
    for(int i=0;host->h_aliases[i];i++){
        printf("Aliases %d %s \n",i+1,host->h_aliases[i]);
    }
    //地址族 h_addrtype
    printf("Address type: %s \n",(host->h_addrtype==AF_INET)?"AF_INET":"AF_INET6");
    //32位 ip地址 h_addr_list[i]
    for(int i=0;host->h_addr_list[i];i++){
        printf("IP addr %d %s \n",i+1,inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }   
    return 0;   
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}