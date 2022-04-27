#include<stdio.h>
#include<pthread.h>
#include<unistd.h>

//创建thread 比较执行流

void* thread_main(void* arg);

int main(int argc,char* argv[]){
    pthread_t t_id;
    int thread_parm=5;
    //创建成功返回0，失败返回其他值
    if(pthread_create(&t_id,NULL,thread_main,(void*)&thread_parm)!=0){
        puts("pthread_create() error");
        return -1;
    }
    sleep(2);
    puts("end of main");

    return 0;
}

void* thread_main(void* arg){
    int cnt=*((int*)arg);   //强制转换为int*类型，再取出数值
    for(int i=0;i<cnt;i++){
        sleep(1);
        puts("running thread");
    }
    return NULL;
}