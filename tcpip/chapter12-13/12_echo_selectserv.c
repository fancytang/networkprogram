#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/select.h>

/**
echo 服务器，基于I/O复用的并发服务器
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

    fd_set reads,cpy_reads;
    struct timeval timeout;
    int fd_max,fd_num;
    
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

    FD_ZERO(&reads);
    FD_SET(serv_sock,&reads);
    fd_max=serv_sock;  //首先监听服务器

    while(1){
        cpy_reads=reads;
        timeout.tv_sec=5;
        timeout.tv_usec=5000;

        //开始监视
        if((fd_num=select(fd_max+1,&cpy_reads,0,0,&timeout))==-1){
            break;
        }
        if(fd_num==0){
            continue;
        }

        for(int i=0;i<fd_max+1;i++){
            if(FD_ISSET(i,&cpy_reads)){  //寻找发送变化的fd
                if(i==serv_sock){  //如果是serv_sock 受理连接请求
                    clnt_addr_size=sizeof(clnt_addr);
                    clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                    FD_SET(clnt_sock,&reads);  //注册一个clnt_sock
                    if(fd_max<clnt_sock){
                        fd_max=clnt_sock;
                    }
                    printf("Connect clinet %d \n",clnt_sock);
                }else{    //不是服务端套接字时,客户端
                    str_len=read(i,message,BUF_SIZE);
                    if(str_len==0){     //没有传输消息
                        FD_CLR(i,&reads);   //清除注册信息
                        close(i);
                        printf("close clinet:%d \n",i);
                    }else{
                        write(i,message,str_len);
                    }
                }
            }
        }
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
