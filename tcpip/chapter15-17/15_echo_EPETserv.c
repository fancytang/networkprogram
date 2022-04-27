#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>  
//为了引入 errno变量 <error.h>

#define BUF_SIZE 4
#define EPOLL_SIZE 50 // 将size调小

/**
epoll实现I/O复用, 边缘触发
**/

//边缘触发中一定要采用非阻塞read/write函数
void setnoblockingmode(int fd);

void error_handling(char *message);

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    int str_len, i;
    char buf[BUF_SIZE];

    struct epoll_event *ep_events;
    struct epoll_event event;
    int event_cnt;
    int epfd;  //epoll的fd

    if (argc != 2)
    {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    //创建epoll例程，参数为epoll实例大小，可以忽略这个参数，填入的参数为操作系统参考
    epfd=epoll_create((EPOLL_SIZE));
    ep_events=malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    setnoblockingmode(serv_sock); //非阻塞read/write函数
    
    event.events=EPOLLIN;//需要读取数据的情况
    event.data.fd=serv_sock;
    
    //在例程内部注册监视对象fd
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event);

    while(1){
        //获取改变了的文件描述符，返回数量,最后一个参数-1表示一直等待知道发生事件
        event_cnt=epoll_wait(epfd,ep_events,EPOLL_SIZE,-1);
        if(event_cnt==-1){
            puts("epoll_wait() error");
            break;
        }

        puts("return epoll_wait");

        for(int i=0;i<event_cnt;i++){
            if(ep_events[i].data.fd==serv_sock){ //客户端请求连接时
                adr_sz=sizeof(clnt_adr);
                clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_adr,&adr_sz);
                
                setnoblockingmode(clnt_sock);    //将 accept 创建的套接字改为非阻塞模式
                event.events=EPOLLIN|EPOLLET;  //改为边缘触发
                event.data.fd=clnt_sock;//把客户端套接字添加进去
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sock,&event);
                printf("connected clinet: %d \n",clnt_sock);
            }else{ //是客户端套接字时
                while(1){   //需要读取输入缓冲中所有数据
                    str_len=read(ep_events[i].data.fd,buf,BUF_SIZE);
                    if(str_len==0){
                        //从epoll中删除套接字
                        epoll_ctl(epfd,EPOLL_CTL_DEL,ep_events[i].data.fd,NULL);
                        close(ep_events[i].data.fd);
                        printf("closed clinet: %d \n",ep_events[i].data.fd);
                        break;//跳出
                    }else if(str_len<0){
                        if(errno==EAGAIN){  //read 返回-1 且 errno 值为 EAGAIN ，意味读取了输入缓冲的全部数据
                            break;  
                        }
                    }else{
                        //发送数据
                        write(ep_events[i].data.fd,buf,str_len);
                    }
                }
            }
        }

    }
    close(serv_sock);
    close(epfd);

    return 0;
}

//边缘触发中一定要采用非阻塞read/write函数
void setnoblockingmode(int fd){
    int flag=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
