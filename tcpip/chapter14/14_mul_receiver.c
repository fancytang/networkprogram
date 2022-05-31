#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

#define BUF_SIZE 100

//实现多播 UDP 要加入多播组的主机地址

void error_handling(char *message);

int main(int argc,char *argv[]){
    int recv_sock;
    struct sockaddr_in send_addr;
    char buf[BUF_SIZE];
    int str_len;

    //多播组结构体
    struct ip_mreq join_adr;
    
    if(argc!=3){
        printf("Usage: %s <GroupID> <ip> \n",argv[0]);
        exit(1);
    }

    recv_sock=socket(PF_INET,SOCK_DGRAM,0);
    memset(&send_addr,0,sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(INADDR_ANY); //必须将IP地址设置为多播地址
    send_addr.sin_port = htons(atoi(argv[2]));

    if(bind(recv_sock,(struct sockaddr*)&send_addr,sizeof(send_addr))==-1){
        error_handling("bind() error");
    }

    //结构体初始化
    join_adr.imr_multiaddr.s_addr=inet_addr(argv[1]);  //组ip
    join_adr.imr_interface.s_addr=htonl(INADDR_ANY);    //主机ip

    //加入多播组
    setsockopt(recv_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(void*)&join_adr,sizeof(join_adr));

    while(1){
        //通过 recvfrom 函数接受多播数据。如果不需要知道传输数据的主机地址信息，可以向recvfrom函数的第5 6参数分贝传入 NULL 0
        str_len=recvfrom(recv_sock,buf,BUF_SIZE-1,0,NULL,0);
        if(str_len<0){
            break;
        }
        buf[str_len]=0;
        fputs(buf,stdout);
    }
    
    close(recv_sock);
    return 0;
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

