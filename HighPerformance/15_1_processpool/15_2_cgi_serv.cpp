#include<iostream>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<sys/epoll.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<signal.h>

#include "15_1_process_pool.h"

//用于处理客户端CGI请求的类，它可以作为processpool类的模板参数
class cgi_conn{
public:
    cgi_conn(){}
    ~cgi_conn(){}
    //初始化客户连接，清空读缓冲区
    //init(m_epollfd,connfd,clinet_address);
    void init(int epollfd,int sockfd,const sockaddr_in &client_addr){
        m_epollfd=epollfd;
        m_sockfd=sockfd;
        m_address=client_addr;
        memset(m_buf,'\0',BUFFER_SIZE);
        m_read_idx=0;
    }

    void process(){
        int idx=0;
        int ret=-1;
        while(1){
            ret=recv(m_sockfd,m_buf+idx,BUFFER_SIZE-1,0);
            if(ret<0){
                if(errno!=EAGAIN){
                    removefd(m_epollfd,m_sockfd);
                }
                break;
            }
            //如果对方关闭连接，则服务器也关闭连接
            else if(ret==0){
                removefd(m_epollfd,m_sockfd);
                break;
            }
            else{
                m_read_idx+=ret;
                fprintf(stdout,"user content is: %s\n",m_buf);
                 //如果遇到字符“\r\n”， 则开始处理客户请求
                for(;idx<m_read_idx;idx++){
                    if((idx>=1)&&(m_buf[idx-1]=='\r')&&(m_buf[idx]=='\n')){
                        break;
                    }
                }
                //如果没有遇到字符“\r\n”，则需要读取更多客户数据
                if(idx==m_read_idx){
                    continue;
                }
                m_buf[idx-1]='\0';

                char *file_name=m_buf;
                //判断文件是否存在
                ////判断客户要运行的CGI程序是否存在
                if(access(file_name,F_OK)==-1){
                    removefd(m_epollfd,m_sockfd);
                    break;
                }

                //创建子进程来执行CGI程序
                ret=fork();
                if(ret==-1){
                    removefd(m_epollfd,m_sockfd);
                    break;
                }else if(ret>0){
                    //父进程只需关闭连接
                    removefd(m_epollfd,m_sockfd);
                }else{
                    //子进程将标准输出定向到m_sockfd, 并执行CGI程序
                    close(STDOUT_FILENO);
                    dup(m_sockfd);
                    //把当前进程替换为一个新进程
                    execl(m_buf,m_buf,(char*)0);
                    exit(0);
                }
            }
        }
    }
private:
    static const int BUFFER_SIZE=1024;
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buf[BUFFER_SIZE];
    //标记读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
};

int cgi_conn::m_epollfd=-1;

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

    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    assert(serv_sock >= 0);

    int ret = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret != -1);

    ret = listen(serv_sock, 5);
    assert(ret != -1);

    processpool<cgi_conn> *pool=processpool<cgi_conn>::create(serv_sock);
    if(pool){
        pool->run();
        delete pool;
    }

    close(serv_sock);//main 函数创建了文件描述符listenfd，那么就由它亲自关闭
    return 0;
}
