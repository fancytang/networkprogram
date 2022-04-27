#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//验证直接半关闭会出现问题

#define BUF_SIZE 1024

int main(int argc, char *argv[]){
    int sock;
    char buf[BUF_SIZE];
    struct sockaddr_in serv_addr;

    FILE *writefp;
    FILE *readfp;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1){
        printf("connect() error");
        exit(1);
    }

    writefp=fdopen(sock,"w");
    readfp=fdopen(sock,"r");

    while(1){
        if(fgets(buf,sizeof(buf),readfp)==NULL){  //收到server的EOF会退出
            break;
        }
        fputs(buf,stdout);
        fflush(stdout);
    }

    fputs("From clinet: Thank you \n",writefp);
    fflush(stdout);
    fclose(writefp);
    fclose(readfp);

    return 0;
}
