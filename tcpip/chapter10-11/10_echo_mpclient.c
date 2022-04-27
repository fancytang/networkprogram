#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>

//基于进程实现并发服务器

#define BUF_SIZE 100

void error_handling(char *message);

int main(int argc,char * argv[]){

    int clnt_sock;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];
    int str_len;

    if(argc!=3){
        printf("Usage : %s <IP> <port> \n,",argv[0]);
        exit(1);
    }

    clnt_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("connect() error");
    }else{
        puts("Connected...");
    }

    while(1){
        fputs("Input message(Q to quit): ",stdout);
        fgets(buf,BUF_SIZE,stdin);

        if(!strcmp(buf,"q\n")||!strcmp(buf,"Q\n")){
            break;
        }

        write(clnt_sock,buf,strlen(buf));
        str_len=read(clnt_sock,buf,BUF_SIZE-1);
        buf[str_len]=0;
        printf("Message from server: %s \n",buf);
    }

    close(clnt_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}