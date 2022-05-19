#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "locker.h"
#include "thread_pool.h"
#include "http_conn.h"

#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000

extern void addfd(int epollfd,int fd,bool one_shot);
extern void removefd(int epollfd,int fd);

void addsig(int sig,void(handler)(int),bool restart=true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler=handler;
    if(restart){
        sa.sa_flags|=SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}


void show_error(int connfd,const char *info){
    printf("%s",info);
    send(connfd,info,sizeof(info),0);
    close(connfd);
}

int main(int argc, const char *argv[])
{
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;
    
    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }


    addsig(SIGPIPE,SIG_IGN);

    threadpool<http_conn> *pool=NULL;
    try{
        pool=new threadpool<http_conn>;
    }catch (...){
        return 1;
    }

    http_conn *users=new http_conn[MAX_FD];
    assert(users);
    int user_count=0;

    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    assert(serv_sock >= 0);

    struct linger tmp={1,0};
    setsockopt(serv_sock,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    int ret = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret>=0);

    ret = listen(serv_sock, 5);
    assert(ret>=0);

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd=epoll_create(5);
    assert(epollfd!=-1);
    addfd(epollfd,serv_sock,false);
    http_conn::m_epollfd=epollfd;

    while(1){
        int number=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number<0)&&(errno!=EINTR)){
            printf("epoll failuer\n");
            break;
        }

        for(int i=0;i<number;i++){
            int sockfd=events[i].data.fd;
            if(sockfd==serv_sock){
                clnt_addr_size=sizeof(clnt_addr);
                int connfd=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                if(connfd<0){
                    printf("errno is: %d\n",errno);
                    continue;
                }

                if(http_conn::m_user_count>=MAX_FD){
                    show_error(connfd,"Internal server busy");
                    continue;
                }

                users[connfd].init(connfd,clnt_addr);
            }

            else if(events[i].events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR)){
                users[sockfd].close_conn();
            }

            else if(events[i].events&EPOLLIN){
                if(users[sockfd].read()){
                    pool->append(users+sockfd);
                }else{
                    users[sockfd].close_conn();
                }
            }

            else if(events[i].events&EPOLLOUT){
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }

            else{}
        }
    }

    close(epollfd);
    close(serv_sock);
    delete [] users;
    delete pool;
    return 0;
}

