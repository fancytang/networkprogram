#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

//基于TCP的半关闭，传输文件内容

#define BUF_SIZE 100

void error_handling(char *message);

int main(int argc,char *argv[]){
    int clnt_sock;

    struct sockaddr_in serv_addr;

    int read_cnt;  //接收的字节数
    char buf[BUF_SIZE];  //缓存区

    FILE *fp=fopen("4_file_receive.dat","wb");

    if(argc!=3){
        printf("Usage: %s <IP> <port>\n",argv[0]);
        exit(1);
    }

    clnt_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));

    while((read_cnt=read(clnt_sock,buf,BUF_SIZE))!=0){
        //size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) 把 ptr 所指向的数组中的数据写入到给定流 stream 中。
        //                            每个元素的大小，以字节为单位
        fwrite((void*)buf,1,read_cnt,fp);
    }

    fputs("Receive data",stdout);
    write(clnt_sock,"Thank you",10);
    fclose(fp);

    close(clnt_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}-=