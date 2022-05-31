#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>

//test1 signal信号注册函数
int signal_test();

//test2 sigaction信号注册函数
int sigaction_test();

//test3 信号处理技术消灭zombie
int signal_zombie_test();

int main(int argc,char *argv[]){
    // signal_test();
    // sigaction_test();
    signal_zombie_test();
}


void timeout(int sig){
    if(sig==SIGALRM){
        puts("Time out!");
    }
    //每隔 2 秒重复产生 SIGALRM 信号
    alarm(2);
}

void keycontrol(int sig){
    if(sig==SIGINT){
        puts("Ctrl+C pressed");
    }
}
int signal_test(){
    signal(SIGALRM,timeout);
    signal(SIGINT,keycontrol);
    alarm(2); ////预约 2 秒候发生 SIGALRM 信号

    for(int i=0;i<3;i++){
        puts("wait...");
        sleep(100);
    }
    
    return 0;
}

int sigaction_test(){
    struct sigaction act;
    act.sa_handler=timeout; //保存函数指针
    sigemptyset(&act.sa_mask);  //将 sa_mask 函数的所有位初始化成0
    act.sa_flags=0; //sa_flags 同样初始化成 0
    sigaction(SIGALRM,&act,0);

    alarm(2);

    for(int i=0;i<3;i++){
        puts("wait...");
        sleep(100);
    }
    return 0;
}

//传递子进程pid
void read_childproc(int sig){
    int status;
    pid_t pid=waitpid(-1,&status,WNOHANG);
    if(WIFEXITED(status)){
        printf("Removed proc id: %d \n",pid);   //子进程pid
        printf("Child send: %d \n",WEXITSTATUS(status));
    }
}

int signal_zombie_test(){
    struct sigaction act;
    act.sa_handler=read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD,&act,0);

    pid_t pid;
    pid=fork();
    if(pid==0){  //child process
        puts("Hi,I'm child process");
        sleep(10);
        exit(12);
    }else{    //parent process  
        printf("Child proc id: %d \n",pid);
        pid=fork();
        if(pid==0){
            puts("Hi,I'm child process");
            sleep(10);
            exit(12);
        }else{
            printf("Child proc id: %d \n",pid);
            for(int i=0;i<5;i++){
                puts("wait...");
                sleep(5);
            }
        }
    }
    return 0;
}
