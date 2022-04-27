#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>

void error_handling(char *message);
void so_type_test(int argc,char *argv[]);
void so_buf_test(int argc,char *argv[]);

//设置端口复用，避免bind error
//SO_REUSEADDR: int opt_val=1; setsockopt(tcp_sock,SOL_SOCKET,SO_REUSEADDR,(void *)&optval,sizeof(optval));

//禁用Nagle算法，不用收到ACK才能发送
//TCP_NODELAY: int opt_val=1; setsockopt(tcp_sock,SOL_SOCKET,TCP_NODELAY,(void *)&optval,sizeof(optval));

int main(int argc,char *argv[]){
    // so_type_test(argc,argv);
    so_buf_test(argc,argv);
    return 0;
}

void so_type_test(int argc,char *argv[]){
    int tcp_sock,udp_sock;

    //查看 SO_SOCKET的SO——TYPE
    int sock_type;
    socklen_t optlen;
    int state;

    printf("-------------TCP-------------\n");
    tcp_sock=socket(PF_INET,SOCK_STREAM,0);
    printf("SOCK_STREAM: %d \n",SOCK_STREAM);

    optlen=sizeof(sock_type);
    state=getsockopt(tcp_sock,SOL_SOCKET,SO_TYPE,(void *)&sock_type,&optlen);
    if(state){
        error_handling("getsockopt() error");
    }
    printf("Socket type: %d \n",sock_type);

    printf("-------------UDP-------------\n");
    udp_sock=socket(PF_INET,SOCK_DGRAM,0);
    printf("SOCK_DGRAM: %d \n",SOCK_DGRAM);

    state=getsockopt(udp_sock,SOL_SOCKET,SO_TYPE,(void *)&sock_type,&optlen);
    if(state){
        error_handling("getsockopt() error");
    }
    printf("Socket type: %d \n",sock_type);
}

void so_buf_test(int argc,char *argv[]){
    int sock;

    sock=socket(PF_INET,SOCK_STREAM,0);

    //查看 SO_SOCKET的SENBUF和RCVBUF
    int send_buf,recv_buf;
    socklen_t optlen;
    int state;

    printf("-------------Read Send Buf-------------\n");
    optlen=sizeof(send_buf);
    state=getsockopt(sock,SOL_SOCKET,SO_SNDBUF,(void *)&send_buf,&optlen);
    if(state){
        error_handling("getsockopt() error");
    }
    printf("Send buf: %d \n",send_buf);

    printf("-------------Read Recv Buf-------------\n");
    optlen=sizeof(recv_buf);
    state=getsockopt(sock,SOL_SOCKET,SO_RCVBUF,(void *)&recv_buf,&optlen);
    if(state){
        error_handling("getsockopt() error");
    }
    printf("Recv buf: %d \n",recv_buf);

    send_buf=1024*3;
    recv_buf=1024*3;

    printf("-------------Set Send Buf-------------\n");
    optlen=sizeof(send_buf);
    state=setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(void *)&send_buf,optlen);
    if(state){
        error_handling("setsockopt() error");
    }
    printf("Send buf: %d \n",send_buf);

    printf("-------------Set Recv Buf-------------\n");
    optlen=sizeof(recv_buf);
    state=setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(void *)&recv_buf,optlen);
    if(state){
        error_handling("getsockopt() error");
    }
    printf("Recv buf: %d \n",recv_buf);

}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}