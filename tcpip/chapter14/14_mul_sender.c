#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

//实现多播 UDP 发送多播消息的服务器

#define BUF_SIZE 100
#define TTL 640

void error_handling(char *message);

int main(int argc,char *argv[]){
    int send_sock;
    struct sockaddr_in mul_adr;

    int time_live=TTL;

    FILE *fp;
    char buf[BUF_SIZE];
    
    if(argc!=3){
        printf("Usage: %s <GroupID> <ip> \n",argv[0]);
        exit(1);
    }

    send_sock=socket(PF_INET,SOCK_DGRAM,0);
    memset(&mul_adr,0,sizeof(mul_adr));
    mul_adr.sin_family = AF_INET;
    mul_adr.sin_addr.s_addr = inet_addr(argv[1]); //必须将IP地址设置为多播地址
    mul_adr.sin_port = htons(atoi(argv[2]));

     //指定套接字中 TTL 的信息
     setsockopt(send_sock,IPPROTO_IP,IP_MULTICAST_TTL,(void*)&time_live,sizeof(time_live));

    if((fp=fopen("14_mul_news.txt","r"))==NULL){
        error_handling("fopen() error");
    }

    //feof()检测流上的文件结束符的函数，如果文件结束，则返回非0值，否则返回0
    while(!feof(fp)){        
        fgets(buf,BUF_SIZE,fp);
        sendto(send_sock,buf,strlen(buf),0,(struct sockaddr*)&mul_adr,sizeof(mul_adr));
        sleep(2);
    }

    fclose(fp);
    close(send_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}
