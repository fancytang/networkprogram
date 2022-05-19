#include<sys/signal.h>
#include<event.h>

void signal_cb(int fd,short event, void* argc){
    struct event_base* base=(struct event_base*)argc;
    struct timeval delay={2,0};
    printf("Caught an interrupt signal;exiting cleanly in two seconds...\n");
    //退出事件循环，两个的区别：break立即退出，exit先执行完回调函数，再退出
    event_base_loopexit(base,&delay);
}

void timeout_cb(int fd,short event,void* argc){
    printf("timeout\n");
}

int main(){
    //创建event_base对象
    struct event_base* base=event_init();
    //创建具体的事件处理器
    struct event* signal_event=evsignal_new(base,SIGINT,signal_cb,base);
    //将EventHandle添加到注册事件队列中，并将其对应的事件添加到事件多路分发器中
    event_add(signal_event,NULL);

    struct timeval tv={1,0};
    struct event* timeout_event=evtimer_new(base,timeout_cb,NULL);
    event_add(timeout_event,&tv);

    //执行事件循环
    event_base_dispatch(base);

    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);

    return 0;
}