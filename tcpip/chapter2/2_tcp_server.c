#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>

//tcp socket 不存在数据边界

void errorHandling(const char* message);

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }
    //1.创建服务器socket
    int sockServ = socket(PF_INET, SOCK_STREAM, 0);

    //2.分配地址信息
    //首先封装结构体
    struct sockaddr_in sockServAddr;
    /** C库函数 void *memset(void *str, int c, size_t n) 
     *  复制字符 c（一个无符号字符）到参数 str 所指向的字符串的前 n 个字符。
     *  <string.h>
    **/
    memset(&sockServAddr, 0, sizeof(sockServAddr));
    sockServAddr.sin_family = AF_INET;
    sockServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockServAddr.sin_port = htons(atoi(argv[1]));

    if(bind(sockServ, (struct sockaddr*)& sockServAddr, sizeof(sockServAddr)) ==-1){
        errorHandling("bind() error!");
    }

    if(listen(sockServ, 5) == -1){
        errorHandling("listen() error!");
    }

    struct sockaddr_in sockClientAddr;
    socklen_t clientAddrLen = sizeof(sockClientAddr);

    int sockClient = accept(sockServ, (struct sockaddr*)& sockClientAddr, &clientAddrLen);
    if(sockClient == -1){
        errorHandling("accept() error!");
    }
    else{
        puts("New Client connected...");
    }

    char message[] = "Hello World!";

    //1次调用write
    write(sockClient, message, strlen(message));

    close(sockClient);
    close(sockServ);

    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}