#define _GNU_SOURCE 1

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<poll.h>
#include<assert.h>
#include <fcntl.h>


#define BUFFER_SIZE 64

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

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        printf("connection failed \n");
        close(sock);
        return 1;
    }

    char *buf=(char*)"cgi\r\n";
    char recvbuf[BUFFER_SIZE];

    send(sock,buf,sizeof(buf),0);

    sleep(1);

    int ret=recv(sock,recvbuf,BUFFER_SIZE,0);
    for(int i=0;i<ret;i++){
        printf("%c",recvbuf[i]);
    }

    printf("\n");
    
    close(sock);

    return 0;
}

