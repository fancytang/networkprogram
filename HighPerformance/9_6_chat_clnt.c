#define _GNU_SOURCE 1

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<poll.h>
#include <fcntl.h>

//聊天室
//poll同时监听用户输入和网络连接，并用splice将用户输入定向到网络连接上实现零拷贝

#define BUF_SIZE 100

void error_handling(char *msg);

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port> \n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    struct pollfd fds[2];
    //fd=0 标准输入
    fds[0].fd=0;
    fds[0].events=POLLIN;
    fds[0].revents=0;
    //fd=sock 可读

    //POLLRDHUP sock的一端关闭了连接，或者是写端关闭了连接，_GNU_SOURCE 可以用来判断链路是否发生异常
    fds[1].fd=sock;
    fds[1].events=POLLIN|POLLRDHUP;
    fds[1].revents=0;

    char buf[BUF_SIZE];

    int pipefd[2];
    int ret=pipe(pipefd);
    if(ret==-1){
        error_handling("pipe() error");
    }

    while(1){
        ret=poll(fds,2,-1);
        if(ret<0){
            fprintf(stderr, "poll failure\n");
            break;
        }
        //关闭一端连接
        if(fds[1].revents & POLLRDHUP){
            fprintf(stdout, "server close the connection\n");
            break;
        }
        //数据接收
        else if(fds[1].revents & POLLIN){
            memset(buf, '\0', sizeof(BUF_SIZE));
            recv(fds[1].fd, buf, BUF_SIZE-1, 0);
            fprintf(stdout, "%s\n", buf);
        }

        //标准输入就绪,从标准输入重定向的网络连接
        if(fds[0].revents & POLLIN){            
            ret=splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
            ret=splice(pipefd[0],NULL,sock,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(sock);

    return 0;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}