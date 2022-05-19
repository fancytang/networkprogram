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
#include<poll.h>

//聊天室 server同时监听和连接socket。并使用牺牲空间换事件的策略来提高性能

#define BUF_SIZE 1024
#define USER_LIMIT 5 /*最大用户数量*/
#define FD_LIMIT 65535  //限制client_data对象数量，poll最大可65536


//对于每个clinet 有addr、有将data写入cinet的位置、有缓存buf
struct client_data{
    struct sockaddr_in address;
    char *write_buf;
    char buf[BUF_SIZE];
};

int setnonblocking(int fd);
void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    char buf[BUF_SIZE];

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

     /*创建users数组，
     * 分配FD_LIMIT个client_data对象。可以预期：每个可能的socket
     * 连接可以获得一个这样的对象，
     * 并且socket的值可以直接用来索引（作为数组的下标)socket连接对应的client_data对象，
     * 这是将socket和客户数据关系的简单而高效的方式
     */
    struct client_data *users=malloc(sizeof(struct client_data)*FD_LIMIT);

    struct pollfd fds[USER_LIMIT+1];
    int user_cnt=0;
    for(int i=0;i<=USER_LIMIT;i++){
        fds[i].fd=-1;
        fds[i].events=0;
    }

    fds[0].fd=serv_sock;
    fds[0].events=POLLIN|POLLERR;
    fds[0].revents=0;

    while(1){
        int ret=poll(fds,user_cnt+1,-1);

        if(ret<1){
            printf("poll failure\n");
            break;
        }


        for(int i=0;i<user_cnt+1;i++){
            //对于new clinet connection
            if((fds[i].fd==serv_sock)&&(fds[i].revents&POLLIN)){
                clnt_addr_size=sizeof(clnt_addr);
                int clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
                if(clnt_sock==-1){
                    printf("errno is: %d\n",errno);
                    continue;
                }

                /*如果请求太多， 则关闭新到的连接 */
                if(user_cnt>=USER_LIMIT){
                    const char *info="too many users\n";
                    fprintf(stdout, "%s\n", info);
                    send(clnt_sock,info,sizeof(info),0);
                    close(clnt_sock);
                    continue;
                }

                /*对于新的连接，同时修改fds和users数组*/
                user_cnt++;
                users[user_cnt].address=clnt_addr;
                setnonblocking(clnt_sock);

                fds[user_cnt].fd=clnt_sock;
                fds[user_cnt].events=POLLIN | POLLRDHUP | POLLERR;
                fds[user_cnt].revents=0;
                fprintf(stdout, "comes a new user, now here %d users\n", user_cnt);
            }
            //对于错误
            else if(fds[i].revents&POLLERR){
                fprintf(stderr, "get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors,0,100);
                socklen_t length=sizeof(errors);
                //清除错误
                if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length)){
                    fprintf(stderr,"get socket option failed\n");
                }
                continue;
            }
            //如果客户端关闭连接，client也close对应socket
            else if(fds[i].revents&POLLRDHUP){
                users[fds[i].fd]=users[fds[user_cnt].fd];
                fds[i]=fds[user_cnt];
                close(fds[i].fd);
                i--;
                user_cnt--;
                printf("A client left\n");
            }
            //对于 客户端数据接收
            else if(fds[i].revents&POLLIN){
                clnt_sock=fds[i].fd;
                memset(users[clnt_sock].buf,0,BUF_SIZE);
                ret=recv(clnt_sock,users[clnt_sock].buf,BUF_SIZE-1,0);
                fprintf(stdout,"get %d bytes of client data: %s from %d\n",ret,users[clnt_sock].buf,clnt_sock);
                //receive failure 
                if(ret<0){
                    //close connection，把最后一个connection移到此处
                    if(errno!=EAGAIN){
                        close(clnt_sock);
                        users[fds[i].fd]=users[fds[user_cnt].fd];
                        fds[i]=fds[user_cnt];

                        i--;
                        user_cnt--;
                    }
                }else if(ret==0){

                }else{
                    //朝着每一个client发送, 产生POLLOUT事件
                    for(int j=0;j<=user_cnt;j++){
                        if(fds[j].fd==clnt_sock){
                            continue;
                        }
                        fds[j].events|=~POLLIN;
                        fds[j].events|=POLLOUT;
                        users[fds[j].fd].write_buf=users[clnt_sock].buf;
                    }
                }
            }
            //对于 发送, 发送之后仍转回接收状态
            else if(fds[i].revents&POLLOUT){
                clnt_sock=fds[i].fd;
                if(!users[clnt_sock].write_buf){
                    continue;
                }
                ret=send(clnt_sock,users[clnt_sock].write_buf,strlen(users[clnt_sock].write_buf),0);
                users[clnt_sock].write_buf=NULL;
                fds[i].events|=~POLLOUT;
                fds[i].events|=POLLIN;
            }
        }
    }
    free(users);
    close(serv_sock);
    return 0;
}

//将fd设置为非阻塞
int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    int new_opt=old_opt|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_opt);
    return old_opt;
}


void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}