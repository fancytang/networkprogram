#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

//基于进程实现并发服务器，可以保存消息的回声服务器

#define BUF_SIZE 100

void error_handling(char *message);

//读取程序
void read_routine(int sock,char *buf);

//发送程序
void write_routine(int sock,char *buf);

int main(int argc,char *argv[]){
    int clnt_sock;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];
    int str_len;

    pid_t pid;
    
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

    pid=fork();

    if(pid==0){  //child process-write
        write_routine(clnt_sock,buf);
    }else{  //parent process-read
        read_routine(clnt_sock,buf);
    }

    close(clnt_sock);
    return 0;
}

void read_routine(int sock,char *buf){
    while(1){
        int str_len=read(sock,buf,BUF_SIZE);
        if(str_len==0){
            return;
        }
        buf[str_len]==0;
        printf("Message from server: %s \n",buf);
    }
}

void write_routine(int sock,char *buf){
    while(1){
        //fputs("Input message(Q to quit): ",stdout);
        fgets(buf,BUF_SIZE,stdin);

        if(!strcmp(buf,"q\n")||!strcmp(buf,"Q\n")){
            // 向服务器传递EOF，虽然后边有close，但是fork后fd也被复制了，close无法再关闭。
            shutdown(sock,SHUT_WR);
            return;
        }
        write(sock,buf,strlen(buf));
    }
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}