#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

//测试 UDP 具有数据边界
//host1 作为clinet

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

    char msg1[]="Hi!";
    char msg2[]="I'm another UDP host!";
    char msg3[]="Nice to meet you.";

    sendto(clnt_sock,msg1,sizeof(msg1),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    sendto(clnt_sock,msg2,sizeof(msg2),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    sendto(clnt_sock,msg3,sizeof(msg3),0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    close(clnt_sock);   
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}