#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/time.h>

//非阻塞connect select ENINPROGRESS

#define BUF_SIZE 1024

int setnonblocking(int fd);

void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    char buf[BUF_SIZE];

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

    int fdopt=setnonblocking(serv_sock);

    int ret=connect(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(ret==0){  //连接成功，恢复sockfd属性并返回
        fprintf(stdout, "connect with server immediately\n");
        fcntl(serv_sock, F_SETFL, fdopt);
        return serv_sock;
    }else if(errno!=EINPROGRESS){ //不是EINPROCESS错误
        fprintf(stderr, "unblock connect not support\n");
        return -1;
    }

    //EINPROCESS错误，通过select、poll等来监听可写事件，返回后getsockopt来读取和清除error

    fd_set write_fds;
    struct timeval timeout;
    FD_SET(serv_sock,&write_fds);

    timeout.tv_sec=5;
    timeout.tv_usec=0;

    ret=select(serv_sock+1,NULL,&write_fds,NULL,&timeout);
    if(ret<=0){   //没有就绪事件
        fprintf(stderr, "connection timeout\n");
        close(serv_sock);
        return -1;
    }

    if(!FD_ISSET(serv_sock,&write_fds)){
        fprintf(stderr, "no event on sockfd found\n");
        close(serv_sock);
        return -1;
    }

    int error=0;
    socklen_t err_len=sizeof(error);
    //连接不成功
    if(getsockopt(serv_sock,SOL_SOCKET,SO_ERROR,&error,&err_len)<0){
        fprintf(stderr, "get socket option failed\n");
        close(serv_sock);
        return -1;
    }

    if(errno!=0){
        printf("connection failed after select with the error: %d \n", error);
        close(serv_sock);
        return -1;
    }

    //0才连接成功
    fprintf(stdout, "connection ready after select with the socket: %d\n", serv_sock);
    //清除
    fcntl(serv_sock,F_SETFL,fdopt);

    close(serv_sock);
    return 0;
}

//将fd设置为非阻塞
int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old_opt|O_NONBLOCK);
    return old_opt;
}


void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
