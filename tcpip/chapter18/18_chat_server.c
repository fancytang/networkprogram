#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define BUF_SIZE 100
#define MAX_CLIENT 256

void* handle_clnt(void* arg);
void send_msg(char* msg,int len);
void error_handling(char *msg);

int clnt_cnt=0;
int clnt_socks[MAX_CLIENT];
pthread_mutex_t mutex;

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;

    pthread_t t_id;

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    
    pthread_mutex_init(&mutex,NULL); //创建互斥锁

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while(1){
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutex); //上锁
        clnt_socks[clnt_cnt++]=clnt_sock; //写入新连接
        pthread_mutex_unlock(&mutex);    //解锁

         //创建线程为新客户端服务，并且把clnt_sock作为参数传递
        pthread_create(&t_id,NULL,handle_clnt,(void*)&clnt_sock);
         //引导线程销毁，不阻塞
        pthread_detach(t_id);
        //客户端连接的ip地址
        printf("Connected client IP: %s \n",inet_ntoa(clnt_adr.sin_addr));

    }
    close(serv_sock);  

    return 0;
}

void* handle_clnt(void* arg){
    int clnt_sock=*((int*)arg);
    char msg[BUF_SIZE];
    int str_len=0;

    while((str_len=read(clnt_sock,msg,sizeof(msg)))!=0){
        send_msg(msg,str_len);
    }

    //接收到消息为0，代表当前客户端已经断开连接
    pthread_mutex_lock(&mutex);
    for(int i=0;i<clnt_cnt;i++){
        if(clnt_sock==clnt_socks[i]){
            while(i<clnt_cnt-1){
                clnt_socks[i]=clnt_socks[i+1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);
    close(clnt_sock);
    return NULL;
}

void send_msg(char* msg,int len){  //向连接的所有客户端发送消息
    pthread_mutex_lock(&mutex);
    for(int i=0;i<clnt_cnt;i++){
        write(clnt_socks[i],msg,len);
    }
    pthread_mutex_unlock(&mutex);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
