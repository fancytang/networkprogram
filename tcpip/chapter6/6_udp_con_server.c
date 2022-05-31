#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>

//connected UDP socker

#define BUF_SIZE 100

void error_handling(char *message);

int main(int argc,char *argv[]){
    int serv_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    int clnt_addr_size;

    char message[BUF_SIZE];
    ssize_t str_len;
    if(argc!=2){
        printf("Usage: %s <port> \n",argv[0]);
        exit(1);
    }

    serv_sock=socket(AF_INET,SOCK_DGRAM,0);
    if(serv_sock==-1){
        error_handling("bind() error");
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }

    while(1){
        clnt_addr_size=sizeof(clnt_addr);
        str_len=recvfrom(serv_sock,message,BUF_SIZE,0,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        sendto(serv_sock,message,str_len,0,(struct sockaddr*)&clnt_addr,clnt_addr_size);
    }

    close(serv_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}