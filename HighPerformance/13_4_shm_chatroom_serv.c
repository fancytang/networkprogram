#define _GNU_SOURCE 1

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
#include<signal.h>
#include<sys/mman.h>
#include<stdbool.h>
#include<sys/wait.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define PROCESS_LIMET 65535

struct client_data{
    struct sockaddr_in address;    //客户端的socket地址
    int connfd;     //socket文件描述符
    pid_t pid;  //处理这个连接的子进程的PID
    int pipefd[2];  //和父进程通信用的管道
};

int sig_pipefd[2];
int epfd;
int serv_sock;
int shmfd;
static const char* shm_name="/my_shm";
char* share_mem=0;
bool stop_child=false;

//当前客户数量
int user_count=0;
//客户连接数组。进程用客户连接的编号来索引这个数组，即可取得相关的客户连接数据
struct client_data* users=0;
//子进程和客户连接的映射关系。用进程的PID来索引这个数组，即可取得该进程所处理的客户连接的编号
int* sub_process=0;

void error_handling(const char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
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

void sig_handler(int sig){
    int save_errno=errno;
    send(sig_pipefd[1],(char*)&sig,1,0);
    errno=save_errno;
}

void addsig(int sig,void(*handler)(int),bool restart=true){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=handler;
    if(restart){
        sa.sa_flags|=SA_RESTART;
    }
    if(sigfillset(&sa.sa_mask)==-1){
        error_handling("sigaction() error");
    }
}

void child_term_handler(int sig){
    stop_child=true;
}


//子进程运行的函数。参数idx指出该子进程处理的客户连接的编号，users是保存所有客户端连接数据的数组，参数share_mem指出共享内存的起始地址
int run_child(int idx,struct client_data* users,char* share_mem){
    struct epoll_event events[MAX_EVENT_NUMBER];
    //子进程使用I/O复用技术来同时监听两个文件描述符：客户连接socket、与父进程通信的管道文件描述符
    int child_epfd=epoll_create(5);
    if(child_epfd==-1){
        error_handling("epoll_create() error");
    }

    int connfd=users[idx].connfd;
    addfd(child_epfd,connfd);
    int pipefd=users[idx].pipefd[1];
    addfd(child_epfd,pipefd);
    //子进程需要设置自己的信号处理函数
    addsig(SIGTERM,child_term_handler,false);

    int ret=0;

    while(!stop_child){
        int number=epoll_wait(child_epfd, events, MAX_EVENT_NUMBER, -1);
        if((number < 0) && (errno != EINTR)){
            printf("epoll failure\n");
            break;
        }

        for(int i=0;i<number;i++){
            int sockfd = events[i].data.fd;
            //本子进程负责的客户连接有数据到达
            if ((sockfd == connfd) && (events[i].events &EPOLLIN)){
                memset(share_mem+idx*BUFFER_SIZE,0,BUFFER_SIZE);
                ret=recv(connfd,share_mem+idx*BUFFER_SIZE,BUFFER_SIZE-1,0);

                if(ret<0){
                    if(errno!=EAGAIN){
                        stop_child=true;
                    }
                }else if(ret==0){
                    stop_child=true;
                }else{
                    //成功读取客户数据后就通知主进程（通过管道）来处理
                    send(pipefd,(char*)&idx,sizeof(idx),0);
                }

            }
            //主进程通知本进程（通过管道）将第client个客户的数据发送到本进程负责的客户端
            else if((sockfd == pipefd) && (events[i].events & EPOLLIN)){
                int client=0;
                //接收主进程发送来的数据， 即有客户数据到达的连接的编号
                ret = recv(sockfd, (char*)&client, sizeof(client), 0);
                if(ret<0){
                    if(errno!=EAGAIN){
                        stop_child=true;
                    }
                }else if(ret==0){
                    stop_child=true;
                }else{
                    //成功读取客户数据后就通知主进程（通过管道）来处理
                    send(connfd,share_mem+client*BUFFER_SIZE,BUFFER_SIZE,0);
                }

            }else{
                continue;
            }

        }
    }
    close(connfd);
    close(pipefd);
    close(child_epfd);
    return 0;
}

int main(int argc,char *argv[]){
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    if (argc != 2) 
    {
        printf( "usage: %s <port>\n", argv[0]);
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

    user_count=0;
    //避免 不能将 "void *" 类型的值分配到 "task_struct *" 类型的实体
    users=(struct client_data*)malloc(sizeof(struct client_data)*(1+FD_LIMIT));
    sub_process=(int*)malloc(sizeof(int)*(PROCESS_LIMET+1));

    for(int i=0;i<PROCESS_LIMET;i++){
        sub_process[i]=-1;
    }

    epoll_event events[MAX_EVENT_NUMBER];
    epfd=epoll_create(1);
    if(epfd==-1){
        error_handling("epoll_create() error");
    }
    addfd(epfd,serv_sock);

    int ret=socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    if(ret==-1){
        error_handling("socketpair() error");
    }
    setnonblocking(sig_pipefd[1]);
    addfd(epfd,sig_pipefd[0]);

    addsig(SIGCHLD,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGPIPE,sig_handler);

    bool stop_server=false;
    bool terminate=false;

    //创建共享内存，作为所有客户socket连接的缓存
    shmfd=shm_open(shm_name,O_CREAT|O_RDWR,0666);
    if(shmfd==-1){
        error_handling("shm_open() error");
    }

    //ftruncate ()会将参数fd 指定的文件大小改为参数length 指定的大小
    ret=ftruncate(shmfd,USER_LIMIT*BUFFER_SIZE);
    if(ret==-1){
        error_handling("ftruncate() error");
    }

    //mmap将一个文件或者其它对象映射到进程的地址空间 <sys/mman.h>
    /*
    void * mmap (void *addr,留给内核来完成
                size_t len,
                int prot,访问权限PROT_READ（可读） , PROT_WRITE （可写）, PROT_EXEC （可执行）, PROT_NONE（不可访问）
                int flags,MAP_SHARED , MAP_PRIVATE , MAP_FIXED，
                int fd,
                off_t offset);
    */
    share_mem=(char*)mmap(NULL,USER_LIMIT*BUFFER_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
    if(*share_mem==MAP_SHARED){
        error_handling("mmap() error");
    }
    close(shmfd);

    while(!stop_server){
        int number=epoll_wait(epfd,events,MAX_EVENT_NUMBER,-1);
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
                    printf("errno is : %d\n",errno);
                    continue;
                }

                if(user_count>=USER_LIMIT){
                    const char* info="too many users\n";
                    printf("%s",info);
                    send(connfd,info,strlen(info),0);
                    close(connfd);
                    continue;
                }

                users[user_count].address=clnt_addr;
                users[user_count].connfd=connfd;
                //在主进程和子进程之间建立管道
                ret=socketpair(PF_UNIX,SOCK_STREAM,0,users[user_count].pipefd);
                if(ret==-1){
                    error_handling("socketpair() error");
                }

                pid_t pid=fork();
                if(pid<0){
                    close(connfd);
                    continue;
                }else if(pid==0){
                    //发送信号
                    close(epfd);
                    close(serv_sock);
                    close(users[user_count].pipefd[0]);
                    close(sig_pipefd[1]);
                    close(sig_pipefd[0]);
                    //子进程开始处理
                    run_child(user_count,users,share_mem);
                    
                    //解除映射
                    munmap((void*)share_mem,USER_LIMIT*BUFFER_SIZE);
                    exit(0);
                }else{
                    close(connfd);
                    close(users[user_count].pipefd[1]);
                    //接收信号
                    addfd(epfd,users[user_count].pipefd[0]);
                    users[user_count].pid=pid;

                    //记录新的客户连接在数组users中的索引值,建立进程pid和该索引值之间的映射关系
                    sub_process[pid]=user_count;
                    user_count++;
                }
            }else if((sockfd==sig_pipefd[0])&&events[i].events&EPOLLIN){
                //接收到信号
                char signals[1024];
                ret=recv(sig_pipefd[0],signals,sizeof(signals),0);
                if(ret==-1){
                    continue;
                }else if(ret==0){
                    continue;
                }else{
                    for(int i=0;i<ret;i++){
                        switch(signals[i]){
                            //子进程退出，表示有某个客户端关闭了连接
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid=waitpid(-1,&stat,WNOHANG))>0){
                                     //清除第del_user个客户连接使用的相关数据
                                    int del_user=sub_process[pid];
                                    sub_process[pid]=-1;
                                    if((del_user<0)||(del_user>USER_LIMIT)){
                                        continue;
                                    }
                                     //清除第del_user个客户连接使用的相关数据
                                    epoll_ctl(epfd,EPOLL_CTL_DEL,users[del_user].pipefd[0],0);
                                    close(users[del_user].pipefd[0]);
                                    users[del_user]=users[--user_count];
                                    sub_process[users[del_user].pid]=del_user;
                                }
                                if(terminate&&user_count==0){
                                    stop_server=true;
                                }

                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                printf("kill all the child now\n");
                                if(user_count==0){
                                    stop_server=true;
                                    break;
                                }
                                for(int i=0;i<user_count;i++){
                                    int pid=users[i].pid;
                                    kill(pid,SIGTERM);
                                }
                                terminate=true;
                                break;
                            }    
                            default: break;
                        }
                    }
                }
            }
            //某个子进程向父进程写入了数据
            else if(events[i].events&EPOLLIN){
                //读取管道数据， child变量记录了是哪个客户连接有数据到达
                int child=0;
                ret = recv(sockfd, (char *)&child, sizeof(child), 0);
                printf("read data from child accross pipe.\n");
                if (ret == -1) 
                {
                    continue;
                }
                else if (ret == 0) 
                {
                    continue;
                }
                else{
                    //向负责处理第child个客户连接的子进程之外的其他子进程发送消息，通知它们有客户数据要写
                    for(int j=0;j<user_count;j++){
                        if(users[j].pipefd[0]!=sockfd){
                            printf("send data to child accross pipe.\n");
                            printf("%s",(char*)&child);
                            send(users[j].pipefd[0],(char*)&child,sizeof(child),0);
                        }
                    }
                }
            }
        }
    }
    //释放资源
    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(serv_sock);
    close(epfd);
    shm_unlink(shm_name);
    free(users);
    free(sub_process);

    return 0;
}
