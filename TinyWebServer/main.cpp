#include<stdio.h>
#include<stdbool.h>

#include "./http/http_conn.h"
#include "./CGImysql/sql_connection_pool.h"
#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "./timer/lst_timer.h"
#include "./log/log.h"
#include "./log/block_queue.h"

#define SYNLOG //同步写日志
// #define ASYNLOG //异步写日志

// #define serv_sockET //边缘触发非阻塞
#define serv_sockLT //水平触发阻塞

#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000
#define TIMESLOT 5

extern int setnonblocking(int fd);
extern void addfd(int epollfd,int fd,bool one_shot);
extern void removefd(int epollfd,int fd);
extern void modfd(int epollfd,int fd,int ev);

//定时器相关参数
//统一事件源
static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd=0;

void show_error(int connfd,const char* info){
    printf("%s",info);
    send(connfd,info,sizeof(info),0);
    close(connfd);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void timer_handler(){
    timer_lst.tick();
    alarm(TIMESLOT);
}

//timer回调函数，删除非活动连接在socket上的注册事件，并关闭
void cb_func(client_data *user_data){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);

    close(user_data->sockfd);
    http_conn::m_user_count--;

    LOG_INFO("close fd %d",user_data->sockfd);
    Log::get_instance()->flush();
}

void sig_handler(int sig){
    int save_errno=errno;
    long msg=sig;
    send(pipefd[1],(char*)&msg,1,0);
    errno=save_errno;
}

void addsig(int sig,void(handler)(int) , bool restart=true){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=handler;
    if(restart){
        sa.sa_flags|=SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

int main(int argc,char *argv[]){
#ifdef ASYNLOG
    Log::get_instance()->init("ServerLog",2000,800000,8);//异步日志
#endif

#ifdef SYNLOG
    Log::get_instance()->init("ServerLog",2000,800000,0);//同步写日志
#endif
    if(argc!=2){
        printf("Usage: %s port_number\n",basename(argv[0]));
        return 1;
    }

    addsig(SIGPIPE,SIG_IGN);

    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    //创建数据库连接池
    connection_pool *connPool=connection_pool::GetInstance();
    connPool->init("localhost","root","000","test_db",3306,8);
    
    //创建线程池
    threadpool<http_conn> *pool=NULL;
    try{
        pool=new threadpool<http_conn>(connPool);
    }catch(...){
        return 1;
    }

    http_conn *users=new http_conn[MAX_FD];
    assert(users);

    //初始化数据库读取表
    users->initmysql_result(connPool);

    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    assert(serv_sock>=0);

    // struct linger tmp={1,0};
    // setsockopt(serv_sock,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    int flag=1;
    setsockopt(serv_sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));

    int ret=bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    assert(ret>=0);

    ret=listen(serv_sock,5);
    assert(ret>=0);

    //创建内核事件表
    struct epoll_event events[MAX_EVENT_NUMBER];
    epollfd=epoll_create(5);
    assert(epollfd!=-1);

    addfd(epollfd,serv_sock,false);
    http_conn::m_epollfd=epollfd;

    //创建信号管道
    ret=socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    assert(ret!=-1);
    setnonblocking(pipefd[1]);
    addfd(epollfd,pipefd[0],false);

    addsig(SIGALRM,sig_handler,false);
    addsig(SIGTERM,sig_handler,false);

    bool stop_server=false;

    //创建用户定时器
    client_data *users_timer=new client_data[MAX_FD];
    bool timeout=false;
    alarm(TIMESLOT);

    while(!stop_server){
        int number=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number<0)&&(errno!=EINTR)){
            LOG_INFO("%s","epoll failure");
            break;
        }

        for(int i=0;i<number;i++){
            int sockfd=events[i].data.fd;

            if(sockfd==serv_sock){
                clnt_addr_size=sizeof(clnt_addr);

#ifdef serv_sockLT
                clnt_sock=accept(serv_sock,(sockaddr*)&clnt_addr,&clnt_addr_size);

                if(clnt_sock<0){
                    LOG_ERROR("%s:errno is: %d","accept error",errno);
                    continue;
                }

                if(http_conn::m_user_count>=MAX_FD){
                    
                    show_error(clnt_sock,"Internal server busy");
                    LOG_ERROR("%s","Internal server busy");
                    continue;
                }

                users[clnt_sock].init(clnt_sock,clnt_addr);

                //初始化定时器
                users_timer[clnt_sock].address=clnt_addr;
                users_timer[clnt_sock].sockfd=clnt_sock;

                util_timer *timer=new util_timer;
                timer->user_data=&users_timer[clnt_sock];
                timer->cb_func=cb_func;
                time_t cur=time(NULL);
                timer->expire=cur+3*TIMESLOT;

                users_timer[clnt_sock].timer=timer;
                timer_lst.add_timer(timer);
#endif

#ifdef serv_sockET
                while(1){
                    clnt_sock=accept(serv_sock,(sockaddr*)&clnt_addr,&clnt_addr_size);

                    if(clnt_sock<0){
                        LOG_ERROR("%s:errno is: %d","accept error",errno);
                        break;
                    }

                    if(http_conn::m_user_count>=MAX_FD){
                        show_error(clnt_sock,"Internal server busy");
                        LOG_ERROR("%s","Internal server busy");
                        break;
                    }

                    users[clnt_sock].init(clnt_sock,clnt_addr);

                    //初始化定时器
                    users_timer[clnt_sock].address=clnt_addr;
                    users_timer[clnt_sock].sockfd=clnt_sock;

                    util_timer *timer=new util_timer;
                    timer->user_data=&users_timer[clnt_sock];
                    timer->cb_func=cb_func;
                    time_t cur=time(NULL);
                    timer->expire=cur+3*TIMESLOT;

                    users_timer[clnt_sock].timer=timer;
                    timer_lst.add_timer(timer);
                }
                continue;
#endif
            }

            //处理信号
            else if((sockfd==pipefd[0])&&(events[i].events&EPOLLIN)){
                char signals[1024];
                int sig;

                ret=recv(pipefd[0],signals,sizeof(signals),0);
                if(ret==-1){
                    continue;
                }
                else if(ret==0){
                    continue;
                }
                else{
                    for(int i=0;i<ret;i++){
                        switch(signals[i]){
                            case SIGALRM:
                            {
                                timeout=true;
                                break;
                            }
                            case SIGTERM:
                            {
                                stop_server=true;
                            }
                        }
                    }
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                timer->cb_func(&users_timer[sockfd]);
                if (timer)
                {
                    timer_lst.del_timer(timer);
                }
            }

            //处理客户连接上接收的数据
            else if(events[i].events&EPOLLIN){
                util_timer *timer=users_timer[sockfd].timer;

                if(users[sockfd].read_once()){
                    LOG_INFO("deal with the client(%s)",inet_ntoa(users[sockfd].get_address()->sin_addr));
                    Log::get_instance()->flush();
                    
                    //若监测到读事件，将该事件放入请求队列
                    pool->append(users+sockfd);

                    //调整定时器
                    if(timer){
                        time_t cur=time(NULL);
                        timer->expire=cur+3*TIMESLOT;
                        LOG_INFO("%s","adjust timer once");
                        Log::get_instance()->flush();
                        timer_lst.adjust_timer(timer);
                    }
                }else{
                    timer->cb_func(&users_timer[sockfd]);
                    if(timer){
                        timer_lst.del_timer(timer);
                    }
                }
            }
            
            else if(events[i].events&EPOLLOUT){
                util_timer *timer=users_timer[sockfd].timer;
                if(users[sockfd].write()){
                    LOG_INFO("send data to the client(%s)",inet_ntoa(users[sockfd].get_address()->sin_addr));
                    Log::get_instance()->flush();

                    if(timer){
                        time_t cur=time(NULL);
                        timer->expire=cur+3*TIMESLOT;
                        LOG_INFO("%s","adjust timer once");
                        Log::get_instance()->flush();
                        timer_lst.adjust_timer(timer);
                    }
                }else{
                    timer->cb_func(&users_timer[sockfd]);
                    if(timer){
                        timer_lst.del_timer(timer);
                    }  
                }
            }
        }

        //最后处理定时时间，因为IO事件有更高的优先级，这会导致定时任务不能准时执行
        if(timeout){
            timer_handler();
            timeout=false;
        }
    }
    close(epollfd);
    close(serv_sock);
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    delete [] users_timer;
    delete pool;
    return 0;
}


