#ifndef __LST_TIMER_H
#define __LST_TIMER_H

#include<arpa/inet.h>
#include<netinet/in.h>
#include<time.h>
#include "../log/log.h"

class util_timer;
struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    util_timer():prev(NULL),next(NULL){}

public:
    time_t expire;
    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst{
public:
    sort_timer_lst():head(NULL),tail(NULL){}
    ~sort_timer_lst(){
        util_timer *temp=head;
        while(temp){
            head=temp->next;
            delete temp;
            temp=head;
        }
    }
    void add_timer(util_timer *timer){
        if(!timer){
            return;
        }
        if(!head){
            head=tail=timer;
            return;
        }
        if(timer->expire<head->expire){
            timer->next=head;
            head->prev=timer;
            head=timer;
            return;
        }
        add_timer(timer,head);

    }

    void adjust_timer(util_timer *timer){
        if(!timer){
            return;
        }
        util_timer *temp=timer->next;
        if(!temp||timer->expire<temp->expire){
            return;
        }
        if(timer==head){
            head=head->next;
            head->prev=NULL;
            timer->next=NULL;
            add_timer(timer,head);
        }
        else{
            timer->next->prev=timer->prev;
            timer->prev->next=timer->next;
            add_timer(timer,timer->next);
        }
    }

    void del_timer(util_timer *timer){
        if(!timer){
            return;
        }
        if((timer==head)&&(timer==tail)){
            delete timer;
            head=NULL;
            tail=NULL;
            return;
        }

        if(timer==head){
            head=head->next;
            head->prev=NULL;
            delete timer;
            return;
        }

        if(timer==tail){
            tail=tail->prev;
            tail->next=NULL;
            delete timer;
            return;
        }

        timer->next->prev=timer->prev;
        timer->prev->next=timer->next;
        delete timer;
    }

    void tick(){
        if(!head){
            return;
        }
        LOG_INFO("%s","timer tick");
        Log::get_instance()->flush();
        time_t cur_time=time(NULL);
        util_timer *temp=head;

        while(temp){
            if(cur_time<temp->expire){
                break;
            }

            temp->cb_func(temp->user_data);
            head=temp->next;
            
            if(head){
                head->prev=NULL;
            }

            delete temp;
            temp=head;
        }
    }

private:
    void add_timer(util_timer *timer,util_timer *lst_head){
        util_timer *prev=lst_head;
        util_timer *temp=lst_head->next;

        while(temp){
            if(timer->expire<temp->expire){
                prev->next=timer;
                timer->next=temp;
                temp->prev=timer;
                timer->prev=prev;
                break;
            }
            prev=temp;
            temp=temp->next;
        }

        if(!temp){
            prev->next=timer;
            timer->next=NULL;
            timer->prev=prev;
            tail=timer;
        }
    }
private:
    util_timer *head;
    util_timer *tail;
};


#endif