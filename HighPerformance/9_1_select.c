#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/select.h>

//select同时接收 ordinary data and oob data

#define BUF_SIZE 1024

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

    //分配IP地址和端口号
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    //进入等待连接请求状态
    if(listen(serv_sock,5)==-1){
        error_handling("listen() error");
    }

    clnt_addr_size=sizeof(clnt_addr);
    clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
    if(clnt_sock==-1){
        error_handling("accept() error");
    }

    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1){
        memset(&buf,0,sizeof(buf));
        //每次select 会修改fd集合，要再次传入
        FD_SET(clnt_sock,&read_fds);
        FD_SET(clnt_sock,&exception_fds);

        int ret=select(clnt_sock+1,&read_fds,NULL,&exception_fds,NULL);

        if(ret<0){
            printf("selection failure\n");
            break;
        }

        //对于可读事件
        if(FD_ISSET(clnt_sock,&read_fds)){
            ret=recv(clnt_sock,buf,sizeof(buf)-1,0);
            if(ret<=0){
                break;
            }
            printf("Get %d bytes of normal data:%s \n",ret,buf);
        }
        /*对于异常事件，采用带MSG_OOB标志的recv函数读取带外数据 */
        else if(FD_ISSET(clnt_sock,&exception_fds)){
            ret=recv(clnt_sock,buf,sizeof(buf)-1,MSG_OOB);
            if(ret<=0){
                break;
            }
            printf("Get %d bytes of oob data:%s \n",ret,buf);
        }
    }
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
