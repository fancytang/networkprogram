#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

/**
理解总流程后自己写的最简单的客户端
**/

#define BUF_SIZE 100

void error_handling(char* message);

int main(int argc,char* argv[]){

    int clnt_sock;
    struct sockaddr_in serv_addr;  //服务器地址
    
    char message[BUF_SIZE];
    int str_len;

    if(argc!=3){
        printf("Usage : %s <IP> <port> \n",argv[0]);
        exit(1);
    }
    clnt_sock=socket(PF_INET,SOCK_STREAM,0);
    if(clnt_sock==-1){
        printf("socket() error");
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    //发送连接请求
    if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("connect() error");
    }

    //read()
    str_len=read(clnt_sock,message,sizeof(message)-1);
    if(str_len==-1){
        error_handling("read() error");
    }

    printf("Message from server: %s \n",message);
    close(clnt_sock);

    return 0;
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}