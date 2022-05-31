#include<stdio.h>
#include<unistd.h>

#define BUF_SIZE 100

//test1 单个管道进行单向通信
void single_pipe_test();

//test2 一个管道进行双向通信
void single_pipe_bi_test();

//test3 两个管道进行双向通信
void two_pipe_bi_test();

int main(int argc,char *argv[]){
    // single_pipe_test();
    // single_pipe_bi_test();
    two_pipe_bi_test();
    return 0;
}

void single_pipe_test(){
    char str[]="Who are you?";
    char buf[BUF_SIZE];

    int fds[2];  // 管道接收(0)和传输(1)数据时所用fd
    pid_t pid;

    //创建管道
    pipe(fds);
    pid=fork();
    if(pid==0){ //child process
        write(fds[1],str,sizeof(str));
    }else{
        read(fds[0],buf,BUF_SIZE);
        puts(buf);
    }
}

void single_pipe_bi_test(){
    char str1[]="Who are you?";
    char str2[]="Thank you for your message!";
    char buf[BUF_SIZE];

    pid_t pid;
    int fds[2];

    pipe(fds);
    pid=fork();
    if(pid==0){
        write(fds[1],str1,sizeof(str1));
        sleep(2);  //保证接下来是是parent先进入管道read
        read(fds[0],buf,BUF_SIZE);
        printf("Child proc output: %s \n",buf);
    }else{
        read(fds[0],buf,BUF_SIZE);
        printf("Parent proc output: %s \n",buf);
        write(fds[1],str2,sizeof(str2));
        sleep(3); //保证接下来是child进入管道读
    }
}

void two_pipe_bi_test(){
    char str1[]="Who are you?";
    char str2[]="Thank you for your message";
    char buf[BUF_SIZE];

    pid_t pid;
    int fds1[2],fds2[2];

    pipe(fds1);
    pipe(fds2);
    pid=fork();

    if(pid==0){
        write(fds1[1],str1,sizeof(str1));
        read(fds2[0],buf,BUF_SIZE);
        printf("Child proc output: %s \n",buf);
    }else{
        read(fds1[0],buf,BUF_SIZE);        
        printf("Parent proc output: %s \n",buf);
        write(fds2[1],str2,sizeof(str2));  
    }
}