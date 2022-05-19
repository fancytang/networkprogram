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
#include<pthread.h>

//epoll的ET的EPOLLONESHOT

#define BUF_SIZE 1024
#define MAX_EVENT_NUMBER 10

//fds结构体的目的是 新开线程接收数据时同时把sockfd和epollfd传过去以便使用
struct fds{
    int sockfd;
    int epollfd;
};

int setnonblocking(int fd);
void addfd(int epfd,int fd,bool enable_et);
void reset_oneshot(int efpd,int fd);
void* worker(void* arg);
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
    //不能注册ONESHOT，否则只能连接一个客户
    addfd(epfd,serv_sock,false);

    while(1){
        int ret=epoll_wait(epfd,events,MAX_EVENT_NUMBER,-1);
        if(ret<0){
            printf("epoll failure\n");
            break;
        }
        //分析就绪事件
        for(int i=0;i<ret;i++){
            int sockfd=events[i].data.fd;
            //new client request
            if(sockfd==serv_sock){
                clnt_addr_size=sizeof(clnt_addr);
                clnt_sock=accept(sockfd,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                if(clnt_sock==-1){
                    error_handling("accept() error");
                }

                //使用ET
                addfd(epfd,clnt_sock,true);
            }
            
            else if(events[i].events & EPOLLIN){  //接收信息
                //新开一个线程来处理，处理完要重置EPOLLONESHOT
                pthread_t thread;
                struct fds new_worker;
                new_worker.epollfd=epfd;
                new_worker.sockfd=sockfd;
                pthread_create(&thread,NULL,worker,(void*)&new_worker);
            }else{
                printf("Something else happened\n");
            }
        }
    }
    
    close(serv_sock);
    return 0;
}

//将fd设置为非阻塞
int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old_opt|O_NONBLOCK);
    return old_opt;
}

//注册事件,是否为条件触发
void addfd(int epfd,int fd,bool oneshot){
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN | EPOLLET;

    if(oneshot){
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

//重置EPOLLONESHOT
void reset_oneshot(int efpd,int fd){
    struct epoll_event event;
    event.events=EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd=fd;
    epoll_ctl(efpd,EPOLL_CTL_MOD,fd,&event);
}

void* worker(void* arg){
    int sockfd = (( struct fds*)arg)->sockfd;
    int epfd = (( struct fds*)arg)->epollfd;

    pthread_t pid = pthread_self();

    printf("Start new thread %u to receive data on fd:%d\n",pid,sockfd);

    char buf[BUF_SIZE];
    memset(&buf,0,sizeof(buf));

    while(1){
        int ret = recv(sockfd, buf, BUF_SIZE-1, 0);
        if(ret==0){
            close(sockfd);
            printf("Foreigner closed the connection\n");
            break;
        }else if(ret<0){  //错误，且errno=EAGAIN
            if(errno==EAGAIN){
                reset_oneshot(epfd,sockfd);
                printf("EAGAIN read later\n");
                break;
            }
        }else{
            printf("thread %u get content: %s\n", pid, buf);
            //printf("thread %u about to sleep\n", pid);
            sleep(5); //模拟数据处理
            //printf("thread %u back from sleep\n", pid);
        }
    }
    printf("End thread receiving data on fd:%d\n",sockfd);
}

void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
