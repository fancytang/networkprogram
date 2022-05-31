#include<stdio.h>
#include<iostream>
#include<mysql/mysql.h>
#include<assert.h>
#include<unistd.h>   //sleep

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/epoll.h>

#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"
#include "./CGImysql/sql_connection_pool.h"
#include "./log/log.h"
#include "./log/block_queue.h"

using namespace std;

//测试单个程序

//test01 测试mysql
namespace name01{
void test01(){
    connection_pool *connPool=connection_pool::GetInstance();
    connPool->init("localhost","root","000","test_db",3306,8);

    MYSQL *mysql=NULL;
    connectionRAII mysqlcon(&mysql,connPool);

    if(mysql_query(mysql,"SELECT username,passwd From user")){
        cout<<"Error:"<<mysql_error(mysql);
        exit(1);
    }

    MYSQL_RES *result=mysql_store_result(mysql);

    int num_fields=mysql_num_fields(result);

    MYSQL_FIELD *fields=mysql_fetch_fields(result);

    while(MYSQL_ROW row=mysql_fetch_row(result)){
        string temp1(row[0]);
        string temp2(row[1]);
        cout<<temp1<<" "<<temp2<<endl;
    }    
}    
}

//test02 测试threadpool
namespace name02{
class http_conn{
public:
    http_conn(){
        mysql=NULL;
    }
    void process(){
        cout<<"processing"<<endl;
    }
public:
    MYSQL *mysql;
};

void test02(){
    connection_pool *connPool=connection_pool::GetInstance();
    connPool->init("localhost","root","000","test_db",3306,8);

    threadpool<http_conn> *pool=new threadpool<http_conn>(connPool);

    http_conn *users=new http_conn[10];
    for(int i=0;i<5;i++){
        cout<<i<<"---";
        pool->append(users);
        sleep(2);
    }
}

}

//test03 测试
namespace name03{
void test03(){
    char *text=(char*)"GET /xxx.html HTTP/1.1";

    char *url=strpbrk(text," \t");
    if(!url){
        cout<<"NO"<<endl;
    }else{
        cout<<url<<endl;
    }

    char *method=text;
    if(strncasecmp(method,"GET",3)==0){        
        cout<<"GET"<<endl;
    }else if(strncasecmp(method,"POST",3)==0){        
        cout<<"POST"<<endl;
    }else{
        cout<<"No method"<<endl;
    }

    url+=strspn(url," \t");
    cout<<url<<endl;

    char *version;
    version=strpbrk(url," \t");
    cout<<version<<endl;
    version+=strspn(version," \t");

    cout<<url<<endl;   

    if(strcasecmp(version,"HTTP/1.1")!=0){
        cout<<"No http"<<endl;;
    }else{
        cout<<version<<endl;
    }

    if(strncasecmp(url,"http://",7)==0){
        url+=7;
        url=strchr(url,'/');
    }

    if(strncasecmp(url,"https://",8)==0){
        url+=8;
        url=strchr(url,'/');
    }
    cout<<url<<endl;

    if(!url||url[0]!='/'){
        cout<<"No"<<endl;
    }else{
        cout<<"Yes"<<endl;
    }

    if(strncasecmp(url,"/ ",2)==0){
        strcat(url,"judge.html");
    }

    cout<<url<<endl;
}
}

/**
//test04 测试http_conn
namespace name04{


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

int test(int argc, const char *argv[])
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

    connection_pool *connPool=connection_pool::GetInstance();
    connPool->init("localhost","root","000","test_db",3306,8);

    threadpool<http_conn> *pool=NULL;
    try{
        pool=new threadpool<http_conn>(connPool);
    }catch (...){
        return 1;
    }

    http_conn *users=new http_conn[MAX_FD];
    assert(users);
    int user_count=0;

    users->init_mysqlresult(connPool);

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
                if(users[sockfd].read_once()){
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

}

**/

// #define SYNLOG 
#define ASYNLOG
namespace name05{
    void test05(){
#ifdef ASYNLOG
        Log::get_instance()->init("ServerLog",2000,800000,8);
#endif

#ifdef SYNLOG
        Log::get_instance()->init("ServerLog",2000,800000,8);
#endif 
        LOG_DEBUG("%s","epoll failure");
        Log::get_instance()->flush();

        LOG_INFO("%s", "adjust timer once");
        Log::get_instance()->flush();

    }
}


int main(int argc,char *argv[]){
    cout<<"-------------------test mysql-----------------"<<endl;
    // name01::test01();
    cout<<"-----------------test threadpool--------------"<<endl;
    // name02::test02();
    cout<<"-----------------test url--------------"<<endl;
    // name03::test03();
    cout<<"-----------------test http--------------"<<endl;
    // name04::test04(argc,argv);
    cout<<"-----------------test log--------------"<<endl;
    name05::test05();
    return 0;
}
