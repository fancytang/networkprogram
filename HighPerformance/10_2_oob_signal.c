#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<signal.h>
#include<stdbool.h>

//信号Signal处理 MSG_OOB

#define BUF_SIZE 1024

static int clnt_sock;

void sig_handler(int sig);
void addsig(int sig);
void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    int option=1;
    setsockopt(serv_sock,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

    //分配IP地址和端口号
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    //进入等待连接请求状态
    if(listen(serv_sock,5)==-1){
        error_handling("listen() error");
    }

    clnt_addr_size=sizeof(clnt_addr);
    clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
    if(clnt_sock<0){
        error_handling("accept() error");
    }

    addsig(SIGURG);
    /* 使用SIGURG信號之前，我们必须设置socket的宿主进程或进程组*/
    fcntl(clnt_sock,F_SETOWN,getpid());

    char buf[BUF_SIZE];
    while(1){
        memset(buf,0,sizeof(buf));
        int ret=recv(clnt_sock,buf,BUF_SIZE-1,0);
        if(ret<=0){
            break;
        }
        printf("get %d bytes of normal data: %s \n",ret,buf);
    }
    close(clnt_sock);
    close(serv_sock);
    return 0; 

}

void sig_handler(int sig){
    fprintf(stdout,"sig_urg function\n");
    int save_errno=errno;
    char buf[BUF_SIZE];
    int ret=recv(clnt_sock,buf,BUF_SIZE-1,MSG_OOB);
    printf("get %d bytes of oob %s \n",ret,buf);
    errno=save_errno;
}

void addsig(int sig){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=sig_handler;
    sa.sa_flags=SA_RESTART;
    sigfillset(&sa.sa_mask);  
    if(sigaction(sig,&sa,NULL)==-1){
        error_handling("sigaction error");
    }
}


void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
