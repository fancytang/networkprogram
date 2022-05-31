#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

//工作线程模型 Worker thread model
//测试模型的临界区错误

#define NUM_THREAD 100

long long num=0;  //全局变量

void* thread_inc(void* arg);
void* thread_des(void* arg);

int main(int argc,char* argv[]){
    pthread_t thread_id[NUM_THREAD];

    printf("sizeof long long: %ld \n", sizeof(long long int));
    
    for(int i=0;i<NUM_THREAD;i++){
        if(i%2){
            pthread_create(&(thread_id[i]),NULL,thread_inc,NULL);  //增加的线程
        }else{
            pthread_create(&(thread_id[i]),NULL,thread_des,NULL);  //减少的线程
        }
    }

    for(int i=0;i<NUM_THREAD;i++){
        pthread_join(thread_id[i],NULL);
    }

    printf("result: %lld \n",num);

    return 0;
}

void* thread_inc(void* arg){
    for(int i=0;i<50000000;i++){
        num+=1;
    }
    return NULL;
}

void* thread_des(void* arg){
    for(int i=0;i<50000000;i++){
        num-=1;
    }
    return NULL;
}