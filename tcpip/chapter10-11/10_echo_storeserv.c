#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>

//基于进程实现并发服务器，可以保存消息的回声服务器

#define BUF_SIZE 100

void error_handling(char *message);

void read_childproc();

int main(int argc,char * argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;

    int str_len;
    char buf[BUF_SIZE];

    pid_t pid;
    struct sigaction act;

    int fds[2];
    pipe(fds);

    if(argc!=2){
        printf("Usage : %s <port> \n,",argv[0]);
        exit(1);
    }

    //sigaction信号处理函数 处理Zombie
    act.sa_handler=read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD,&act,0);

    //新建进程，利用pipe保存数据
    pid=fork();
    if(pid==0){  //child proc
        //user拥有读写的权限
        int fd = open("10_echomsg.txt", O_CREAT|O_APPEND|O_WRONLY,S_IRUSR|S_IWUSR);
        char msgbuf[BUF_SIZE];
        while(read(fds[0],msgbuf,sizeof(msgbuf))){
            write(fd, msgbuf, strlen(msgbuf));
            memset(msgbuf,0,sizeof(msgbuf));
        }
        close(fd);
        //失败的做法
        // FILE *fp=fopen("10_echomsg.txt","wt");
        // char msgbuf[BUF_SIZE];
        // int len;
        // for(int i=0;i<10;i++){
        //     len=read(fds[0],msgbuf,BUF_SIZE);
        //     fwrite((void*)msgbuf,1,len,fp);
        // }
        // fclose(fp);
        return 0;
        
    }

    //创建服务端套接字
    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    //分配IP地址和端口号
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    //进入等待连接请求状态
    if(listen(serv_sock,5)==-1){
        error_handling("listen() error");
    }

    while(1){
        clnt_addr_size=sizeof(clnt_addr);

        clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        if(clnt_sock==-1){
            continue;
        }else{  //新的客户端连接
            puts("new client connected...");
        }

        //new client
        pid=fork();  //此时，父子进程分别带有一个套接字
        if(pid==0){      //创建成功，子进程
            close(serv_sock); //关闭服务器套接字，因为从父进程传递到了子进程
            //接收数据
            while((read(clnt_sock,&buf,BUF_SIZE))!=0){
                //发送数据
                buf[sizeof(buf)]=0;
                write(clnt_sock,&buf,strlen(buf)+1);
                //写入管道
                write(fds[1],buf,strlen(buf)+1);
                memset(buf,0,BUF_SIZE);
            }
            close(clnt_sock);
            puts("clinet disconnected...");
            return 0;
        }else{   //父进程
            //通过 accept 函数创建的套接字文件描述符已经复制给子进程，因为服务器端要销毁自己拥有的
            close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;
}

//子进程结束处理
void read_childproc(){
    pid_t pid;
    int status;
    pid=waitpid(-1,&status,WNOHANG);
    printf("removed proc id: %d \n",pid);
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
