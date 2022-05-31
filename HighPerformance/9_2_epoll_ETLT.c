#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<stdbool.h>

//epoll的ET and LT

#define BUF_SIZE 10
#define MAX_EVENT_NUMBER 10

int setnonblocking(int fd);
void addfd(int epfd,int fd,bool enable_et);
void lt(struct epoll_event* events,int number,int epfd,int listenfd);
void et(struct epoll_event* events,int number,int epfd,int listenfd);
void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    char buf[BUF_SIZE];

    struct epoll_event events[MAX_EVENT_NUMBER];

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

    int epfd=epoll_create(5);
    if(epfd==-1){
        error_handling("epoll_create() error");
    }
    addfd(epfd,serv_sock,true);

    while(1){
        int ret=epoll_wait(epfd,events,MAX_EVENT_NUMBER,-1);
        if(ret<0){
            printf("epoll failure\n");
            break;
        }
        //分析就绪事件
        // lt(events,ret,epfd,serv_sock);
        et(events,ret,epfd,serv_sock);
    }

    
    close(serv_sock);
    return 0;
}

//将fd设置为非阻塞
int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    fcntl(fd,old_opt|O_NONBLOCK);
    return old_opt;
}

//注册事件,是否为条件触发
void addfd(int epfd,int fd,bool enable_et){
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN;

    if(enable_et){
        event.events|=EPOLLET;
    }
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void lt(struct epoll_event* events,int number,int epfd,int listenfd){
    for(int i=0;i<number;i++){
        int sockfd=events[i].data.fd;
        // new client request
        if(sockfd==listenfd){
            struct sockaddr_in clnt_addr;
            socklen_t clnt_addr_size;

            clnt_addr_size=sizeof(clnt_addr);
            int clnt_sock=accept(sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
            if(clnt_sock==-1){
                error_handling("accept() error");
            }

            //不使用ET
            addfd(epfd,clnt_sock,false);
        }else if(events[i].events & EPOLLIN){  //接收信息
            printf("Event trigger once\n");
            char buf[BUF_SIZE];
            int ret=recv(sockfd,buf,BUF_SIZE-1,0);
            if(ret<0){
                close(sockfd);
                continue;
            }
            printf("Get %d bytes of content: %s \n",ret,buf);
        }else{
            printf("Sonmething else happened\n");
        }
    }
}

void et(struct epoll_event* events,int number,int epfd,int listenfd){
    for(int i=0;i<number;i++){
        int sockfd=events[i].data.fd;
        // new client request
        if(sockfd==listenfd){
            struct sockaddr_in clnt_addr;
            socklen_t clnt_addr_size;

            clnt_addr_size=sizeof(clnt_addr);
            int clnt_sock=accept(sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
            if(clnt_sock==-1){
                error_handling("accept() error");
            }

            //使用ET
            addfd(epfd,clnt_sock,true);
        }else if(events[i].events & EPOLLIN){  //接收信息
            printf("Event trigger once\n");
            char buf[BUF_SIZE];
            while(1){
                memset(&buf,0,sizeof(buf));
                int ret=recv(sockfd,buf,BUF_SIZE-1,0);
                if(ret<0){
                    // 判断errno
                    if((errno==EAGAIN)||(errno==EWOULDBLOCK)){
                        printf("Read later\n");
                        break;
                    }
                    close(sockfd);
                    break;
                }else if(ret==0){
                    close(sockfd);
                }else{
                    printf("Get %d bytes of content: %s \n",ret,buf);
                }
            }
        }else{
            printf("Sonmething else happened\n");
        }
    }
}

void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
