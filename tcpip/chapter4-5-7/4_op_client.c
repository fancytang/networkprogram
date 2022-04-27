#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

/**
计算器客户端
**/

#define BUF_SIZE 100
#define OP_SIZE 4  //偏移量，int大小 4byte

void error_handling(char* message);

int main(int argc,char* argv[]){

    int clnt_sock;
    struct sockaddr_in serv_addr;  //服务器地址
    
    int result;  //接收计算结果
    int str_len;  
    int operand_cnt;  
    char opmsg[BUF_SIZE];

    if(argc!=3){
        printf("Usage : %s <IP> <port> \n",argv[0]);
        exit(1);
    }
    clnt_sock=socket(PF_INET,SOCK_STREAM,0);
    if(clnt_sock==-1){
        printf("socket() error");
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    //发送连接请求
    if(connect(clnt_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("connect() error");
    }else{
        puts("Connected......");
    }

    fputs("Please enter the number of operand:",stdout);
    scanf("%d",&operand_cnt); //接收操作数的个数
    opmsg[0]=(char)operand_cnt;//存到msg中

    //接收操作数
    for(int i=0;i<operand_cnt;i++){
        printf("Operand %d: ",i+1);
        scanf("%d",(int*)&opmsg[i*OP_SIZE+1]);
    }
    //接收操作符
    fgetc(stdin);
    fputs("Operator: ",stdout);
    scanf("%c",&opmsg[operand_cnt*OP_SIZE+1]);

    //发送msg
    write(clnt_sock,opmsg,operand_cnt*OP_SIZE+2);
    
    //read()
    str_len=read(clnt_sock,&result,OP_SIZE);
    if(str_len==-1){
        error_handling("read() error");
    }
    
    printf("Operation result: %d \n",result);
    close(clnt_sock);

    return 0;
}

void error_handling(char* message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}