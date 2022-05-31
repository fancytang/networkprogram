#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <fcntl.h>

//TCP client

#define BUF_SIZE 100

void error_handling(char *msg);

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    int ret=0;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port> \n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    char buf[BUF_SIZE];
    char *msg="Hello!\n";

    ret=sendto(sock,msg,sizeof(msg),0,(struct sockaddr *)&serv_addr, sizeof(serv_addr));
    //回声
    socklen_t length=sizeof(serv_addr);
    recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr *)&serv_addr,&length);
    printf("UDP-Receive: %s",buf);

    close(sock);

    return 0;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}