#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<signal.h>
#include<stdbool.h>

//将信号Signal与I/O事件统一，统一事件源
// signal_handler 向 pipe中写入信号值，main while 从pipe中用poll获取 pipe中的信号值 进行处理

#define MAX_EVENT_NUMBER 1024
//传输信号值的管道
static int pipefd[2];

void sig_handler(int sig);
void addsig(int sig);
int setnonblocking(int fd);
void addfd(int epfd,int fd);
void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

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

    int option=1;
    setsockopt(serv_sock,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

    //分配IP地址和端口号
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    //进入等待连接请求状态
    if(listen(serv_sock,5)==-1){
        error_handling("listen() error");
    }

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epfd=epoll_create(5);
    if(epfd==-1){
        error_handling("epoll_create() error");
    }
    addfd(epfd,serv_sock);


    /*使用socketpair创建管道， 注册pipefd[0]上的可读事件*/
    int ret=socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    if(ret==-1){
        error_handling("socketpair() error");
    }
    setnonblocking(pipefd[1]);
    addfd(epfd,pipefd[0]);

    /*设置一些信号的处理函数*/
    addsig(SIGHUP);
    addsig(SIGCHLD);
    addsig(SIGTERM);
    addsig(SIGINT);

    bool stop_server=false;

    while(!stop_server){
        ret=epoll_wait(epfd,events,MAX_EVENT_NUMBER,-1);
        if((ret<0)&&(errno!=EINTR)){
            fprintf(stderr,"epoll wait failure\n");
            break;
        }

        for(int i=0;i<ret;i++){
            int sockfd=events[i].data.fd;

            /*如果就绪的文件描述符是listenfd，则处理新的连接*/
            if(sockfd==serv_sock){
                clnt_addr_size=sizeof(clnt_addr);
                clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                addfd(epfd,clnt_sock);
            }

            /*如果就绪的文件描述符是pipefd[0], 则处理信号*/
            else if((sockfd==pipefd[0])&&(events[i].events&EPOLLIN)){
                char signals[1024];
                ret=recv(pipefd[0],signals,sizeof(signals),0);
                if(ret==-1){
                    continue;
                }else if(ret==0){
                    continue;
                }else{
                     /* 因为每个信号值占1字节，所以按字节来逐个接收信号。我们以SIGTERM
                     * 为例，来说明如何安全地终止服务器主循环
                     */
                    for(int i=0;i<ret;i++){
                        switch(signals[i]){
                            case SIGCHLD:
                            case SIGHUP:continue;
                            case SIGTERM:
                            case SIGINT:
                                stop_server=true;
                                printf("CTRL + C --> close fds\n"); 
                        }
                    }
                }
            }
            else{}
        }
    }
    close(serv_sock);
    close(pipefd[0]);
    close(pipefd[1]);
    return 0; 

}

void sig_handler(int sig){
    int save_errno=errno;
    send(pipefd[1],(char*)&sig,1,0);
    errno=save_errno;
}

void addsig(int sig){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=sig_handler;
    sa.sa_flags=SA_RESTART;
    sigfillset(&sa.sa_mask);  
    if(sigaction(sig,&sa,NULL)==-1){
        error_handling("sigaction error");
    }
}

int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    int new_opt=old_opt|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_opt);
    return old_opt;
}

void addfd(int epfd,int fd){
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
