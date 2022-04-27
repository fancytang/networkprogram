#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

//test1 创建进程 fork()
void fork_test();

//test2 创建僵尸进程
void zombie_test();

//test3 用wait()销毁zombie
int wait_test();

//test4 用waitpid()销毁zombie
int waitpid_test();

int main(int argc,char *argv[]){
    // fork_test();
    // zombie_test();
    // wait_test();
    waitpid_test();
    return 0;
}

//test1 创建进程 fork()
int gval=10;  //glabal 
void fork_test(){
    int lval=20;
    gval++;
    lval+=5;

    pid_t pid;
    pid=fork();
    if(pid==0){  //child process
        gval+=2;
        lval+=2;
        printf("Child process: [%d,%d]\n",gval,lval);
    }else{   //parent process
        gval-=2;
        lval-=2;
        printf("Parent process: [%d,%d]\n",gval,lval);

    }
}

void zombie_test(){
    pid_t pid=fork();
    // 父子进程同时从复制点开始进行
    if(pid==0){  //child
        puts("Hi,I am a child process");
    }else{
        printf("Child process ID: %d\n",pid);
        sleep(30);
    }

    if(pid==0){
        puts("End child process");
    }else{
        puts("End parent process");
    }
}

int wait_test(){
    int status; //保存子pid
    pid_t pid=fork();

    if(pid==0){  //child
        exit(3);
    }else{
        printf("Child PID: %d \n",pid);
        pid=fork();
        if(pid==0){
            exit(7);
        }else{
            printf("Child PID: %d \n",pid);
            
            // two child process 都要销毁
            //子pid保存到status所指向的内存空间，还有其他信息，通过相关宏进行分离。
            wait(&status);            
            if(WIFEXITED(status)){  //正常返回时
                //WEXITSTATUS(status)取得子进程exit()返回的结束代码，一般会先用WIFEXITED 来判断是否正常结束才能使用此宏
                printf("Child send one: %d \n",WEXITSTATUS(status));  //输出子pid
            }

            wait(&status);            
            if(WIFEXITED(status)){  //正常返回时
                printf("Child send two: %d \n",WEXITSTATUS(status));  //输出子pid
            }
            sleep(30);
        }
    }
    return 0;
}

int waitpid_test(){
    int status;
    pid_t pid=fork();

    if(pid==0){
        sleep(15);
        exit(24);
    }else{
        while(!waitpid(-1,&status,WNOHANG)){
            sleep(5);
            puts("sleep 5 secs");
        }

        if(WIFEXITED(status)){
            printf("Child send one: %d \n",WEXITSTATUS(status));
        }
    }
    return 0;
}