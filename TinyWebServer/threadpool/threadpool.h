#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"

#include<exception>
#include<pthread.h>
#include<list>
#include<stdio.h>

template<typename T>
class threadpool{
public:
    threadpool(connection_pool *connPool,int thread_number=8,int max_requests=10000);

    ~threadpool();

    bool append(T *request);

private:
    static void *worker(void *arg);
    void run();

private:
    int m_threads_number;  //线程池中的线程数
    int m_max_requests;    //请求队列中允许的最大请求数
    pthread_t *m_threads;    //描述线程池的数组，其大小为m_thread_number
    list<T *> m_workqueue; //请求队列
    locker m_queuelocker;  //保护请求队列的互斥锁
    sem m_queuestat;       //是否有任务需要处理
    bool m_stop;           //是否结束线程
    connection_pool *m_connPool;  //数据库池

};

template<typename T>
threadpool<T>::threadpool(connection_pool *connPool,int thread_number,int max_requests)
        :m_threads_number(thread_number),m_max_requests(max_requests),m_threads(NULL),m_stop(false),m_connPool(connPool){
    if(thread_number<=0||max_requests<=0){
        throw std::exception();
    }        

    m_threads=new pthread_t[m_threads_number];
    if(!m_threads){
        throw std::exception();
    }

    for(int i=0;i<thread_number;i++){
        //线程创建
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete [] m_threads;
            throw std::exception();
        }

        //线程分离
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}


template<typename T>
threadpool<T>::~threadpool(){
    delete [] m_threads;
    m_stop=true;
}

template<typename T>
bool threadpool<T>::append(T *request){
    m_queuelocker.lock();

    if(m_workqueue.size()>m_max_requests){
        m_queuelocker.unlock();
        return false;
    }

    //添加任务
    m_workqueue.push_back(request);
    m_queuelocker.unlock();

    //信号量提醒有任务要处理
    m_queuestat.post();
    return true;
}

template<typename T>
void *threadpool<T>::worker(void *arg){
    threadpool *pool=(threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(!m_stop){
        m_queuestat.wait();

        m_queuelocker.lock();

        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        T* request=m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        if(!request){
            continue;
        }

        //从数据库连接池取出一个数据库连接
        request->mysql=m_connPool->GetConnection();
        request->process();
        //返回数据库连接
        m_connPool->ReleaseConnection(request->mysql);
    }
}


#endif