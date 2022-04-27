#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

//基于UDP的回声客户端

#define BUF_SIZE 100 

void error_handling(char *message);

int main(int argc,char *argv[]){
    int clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;

    ssize_t str_len;
    int clnt_addr_size;

    char message[BUF_SIZE];

    if(argc!=3){
        printf("Usage: %s <IP> <port> \n",argv[0]);
        exit(1);
    }

    clnt_sock=socket(PF_INET,SOCK_DGRAM,0);
    if(clnt_sock==-1){
        error_handling("socket() error");
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    while(1){
        fputs("Insert message(q to quit):",stdout);
        fgets(message,sizeof(message),stdin);
        if(!strcmp(message,"q\n")||!strcmp(message,"Q\n")){
            break;            
        }
        sendto(clnt_sock,message,sizeof(message),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

        clnt_addr_size=sizeof(clnt_addr);
        str_len=recvfrom(clnt_sock,message,BUF_SIZE,0,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        message[str_len]=0;
        printf("Message from server: %s \n",message);
    }

    close(clnt_sock);   
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}