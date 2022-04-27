#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

/** 标准io
echo 服务器，可以连接5个客户端,但同一时刻只能连接一个
**/

#define BUF_SIZE 1024

void error_handling(char* message);

//argc是命令行总的参数个数  
//argv[]是argc个参数，其中第0个参数是程序的全名，以后的参数是命令行后面跟的用户输入的参数
int main(int argc,char* argv[]){
    int serv_sock;  //连接socket
    int clnt_sock;  //数据IO socket

    struct sockaddr_in serv_addr;  //定义结构体地址，服务端
    struct sockaddr_in clnt_addr;//客户端地址
    socklen_t clnt_addr_size;//clinet addr size

    char message[BUF_SIZE];
    int str_len;//message size

    FILE *readfp;
    FILE *writefp;
    
    if(argc!=2){
        printf("Usage : %s <port>\n",argv[0]);
        exit(1);
    }

    // 创建socket
    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    if(serv_sock==-1){
        error_handling("socket() error");
    }

    //分配IP和端口号，先初始化结构体
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    // int atoi(const char *str) 把参数 str 所指向的字符串转换为一个整数（类型为 int 型）
    //<stdlib.h>
    serv_addr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }

    //转为可接收连接请求的状态
    //连接请求等待队列大小为5
    if(listen(serv_sock,5)==-1){
        error_handling("listen() error");
    }

    clnt_addr_size=sizeof(clnt_addr);

    for(int i=0;i<5;i++){
        //受理连接请求
        clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        if(clnt_sock==-1){
            error_handling("accept() error");
        }else{
            printf("Connect clinet %d \n",i+1);
        }

        
        //传输数据，当接收数据不为空时回传。
        readfp=fdopen(clnt_sock,"r");
        writefp=fdopen(clnt_sock,"w");
        while(!feof(readfp)){
            fgets(message,BUF_SIZE,readfp);
            fputs(message,writefp);
            //fflush()用于清空文件缓冲区，如果文件是以写的方式打开的，则把缓冲区内容写入文件。int fflush(FILE* stream);
            fflush(writefp);
        }

        //关闭连接
        fclose(readfp);
        fclose(writefp);
    }

    close(serv_sock);
    return 0;
}

void error_handling(char* message){
    //stderr -- 标准错误输出设备
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
