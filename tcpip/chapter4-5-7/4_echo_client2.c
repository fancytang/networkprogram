#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

/**
echo 客户端
更改：收到字符串数据后立即读取并输出，而非等待一段时间再接收。
提前确定数据大小
**/

#define BUF_SIZE 100

void error_handling(char* message);

int main(int argc,char* argv[]){

    int clnt_sock;
    struct sockaddr_in serv_addr;  //服务器地址
    
    char message[BUF_SIZE];
    int str_len;
    int recv_len,recv_cnt;

    if(argc!=3){
        printf("Usage : %s <IP> <port> \n",argv[0]);
        exit(1);
    }
    clnt_sock=socket(PF_INET,SOCK_STREAM,0);
    if(clnt_sock==-1){
        error_handling("socket() error");
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    //发送连接请求
    if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("connect() error");
    }else{
        puts("Connected......");
    }

    //不断发送，遇到q/Q停止
    while(1){
        //提示输入信息
        fputs("Input message(Q to quit): ",stdout);
        fgets(message,BUF_SIZE,stdin);

        if(!strcmp(message,"q\n")||!strcmp(message,"Q\n")){
            break;
        }

        str_len=write(clnt_sock,message,strlen(message));

        recv_len=0;
        while(recv_len<str_len){
            //read()
            recv_cnt=read(clnt_sock,message,BUF_SIZE-1);
            if(recv_cnt==-1){
                error_handling("read() error");
            }
            recv_len+=recv_cnt;
        }
        
        message[recv_len]=0;
        printf("Message from server: %s \n",message);

    }

    close(clnt_sock);

    return 0;
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}