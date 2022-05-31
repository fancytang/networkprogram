#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//验证直接半关闭会出现问题

#define BUF_SIZE 1024

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;

    socklen_t clnt_adr_sz;
    char buf[BUF_SIZE] = {0,};

    FILE *readfp;
    FILE *writefp;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))==-1){
        printf("bind() error");
        exit(1);
    }
    listen(serv_sock, 5);

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    readfp=fdopen(clnt_sock,"r");
    writefp=fdopen(clnt_sock,"w");

    fputs("From server: Hi! clinet?\n",writefp);
    fputs("I love all of the world \n",writefp);
    fputs("You are awesome! \n",writefp);
    fflush(writefp);  

    fclose(writefp);   //关闭write流

    fgets(buf,sizeof(buf),readfp);
    fputs(buf,stdout);
    fclose(readfp);

    return 0;
}
