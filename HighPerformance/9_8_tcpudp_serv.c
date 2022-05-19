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

#define BUF_SIZE 1024
#define MAX_EVENT_NUMBER 64

//同时处理TCP和UDP

int setnonblocking(int fd);
void addfd(int epfd,int fd);
void error_handling(char *msg);


int main(int argc,char *argv[]){
    int tcp_sock,udp_sock,clnt_sock;
    struct sockaddr_in address,clnt_addr;
    socklen_t clnt_addr_size;

    char buf[BUF_SIZE];

    struct epoll_event events[MAX_EVENT_NUMBER];

    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    //tcp socket
    tcp_sock=socket(PF_INET,SOCK_STREAM,0);
    if(tcp_sock==-1){
        error_handling("tcp socket() error");
    }
    memset(&address,0,sizeof(address));
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_ANY);
    address.sin_port=htons(atoi(argv[1]));

    int option=1;
    setsockopt(tcp_sock,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));

    int ret=0;

    //分配IP地址和端口号
    if(bind(tcp_sock,(struct sockaddr*)&address,sizeof(address))==-1){
        error_handling("bind() error");
    }
    //进入等待连接请求状态
    if(listen(tcp_sock,5)==-1){
        error_handling("listen() error");
    }

    udp_sock=socket(PF_INET,SOCK_DGRAM,0);
    if(udp_sock==-1){
        error_handling("udp socket() error");
    }
    if(bind(udp_sock,(struct sockaddr*)&address,sizeof(address))==-1){
        error_handling("bind() error");
    }    

    int epfd=epoll_create(5);
    if(epfd==-1){
        error_handling("epoll_create() error");
    }
    addfd(epfd,tcp_sock);
    addfd(epfd,udp_sock);

    while(1){
        int ret=epoll_wait(epfd,events,MAX_EVENT_NUMBER,-1);
        if(ret<0){
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<ret;i++){
            int sockfd=events[i].data.fd;
            // new tcp client request
            if(sockfd==tcp_sock){
                clnt_addr_size=sizeof(clnt_addr);
                int clnt_sock=accept(sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                if(clnt_sock==-1){
                    error_handling("accept() error");
                }

                //使用ET注册fd
                addfd(epfd,clnt_sock);
            }
            //udp 传输数据 数据报 不用建立连接 用sendto recvfrom传递client address
            else if(sockfd==udp_sock){ 
                char buf[BUF_SIZE];
                while(1){
                    memset(&buf,0,sizeof(buf));
                    clnt_addr_size=sizeof(clnt_addr);
                    //<sys/types.h>
                    ret=recvfrom(sockfd,buf,BUF_SIZE-1,0,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                    printf("UDP-Received: %s",buf);
                    if(ret>0){
                        sendto(sockfd,buf,BUF_SIZE-1,0,(struct sockaddr*)&clnt_addr,clnt_addr_size);
                    }
                }
            }
            //tcp 传输数据
            else if(events[i].events&EPOLLIN){
                memset(&buf,0,sizeof(buf));
                while(1){
                    clnt_addr_size=sizeof(clnt_addr);
                    //<sys/types.h>
                    ret=recv(sockfd,buf,BUF_SIZE-1,0);
                    if(ret<0){
                        if((errno==EAGAIN)||(errno==EWOULDBLOCK)){
                            break;
                        }
                        close(sockfd);
                        break;
                    }else if(ret==0){
                        close(sockfd);
                    }else{
                        printf("TCP-Received: %s",buf);
                        send(sockfd,buf,BUF_SIZE-1,0);
                    }
                }                
            }else{
                printf("Sonmething else happened\n");
            }
        }
    }
    
    close(tcp_sock);
    return 0;
}

//将fd设置为非阻塞
int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    fcntl(fd,old_opt|O_NONBLOCK);
    return old_opt;
}

//注册事件,是否为条件触发
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


