#include<stdio.h>
#include<pthread.h>

//工作线程模型 Worker thread model
//开两个线程，线程1计算 1-5之和，线程2计算6-10之和
//会有小问题，临界区小错误，此例程不明显

int sum=0;  //全局变量

void* thread_sum(void* arg);

int main(int argc,char* argv[]){
    pthread_t id_t1,id_t2;
    int range1[]={1,5};
    int range2[]={6,10};

    if(pthread_create(&id_t1,NULL,thread_sum,(void*)range1)!=0){
        puts("pthread_create() error");
        return -1;
    }

    if(pthread_create(&id_t2,NULL,thread_sum,(void*)range2)!=0){
        puts("pthread_create() error");
        return -1;
    }

    pthread_join(id_t1,NULL);
    pthread_join(id_t2,NULL);
    printf("result: %d \n",sum);

    return 0;
}

void* thread_sum(void* arg){
    int start=((int*)arg)[0];
    int end=((int*)arg)[1];

    while(start<=end){
        sum+=start;
        start++;
    }

    return NULL;
}