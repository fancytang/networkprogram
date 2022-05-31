#include<stdio.h>
#include<unistd.h>
#include<sys/select.h>
#include<sys/time.h>

#define BUF_SIZE 100

int main(int argc,char *argv[]){

    fd_set reads,temps;
    struct timeval timeout;

    int result;
    int str_len;

    char buf[BUF_SIZE];

    FD_ZERO(&reads); //初始化变量
    FD_SET(0,&reads); //将文件描述符0对应的位设置为1

    while(1){
        temps=reads;
        timeout.tv_sec=5;
        timeout.tv_usec=0;
        result=select(1,&temps,0,0,&timeout);

        if(result==-1){
            puts("select() error");
            break;
        }else if(result==0){
            puts("Time-out");
        }else{
            if(FD_ISSET(0,&temps)){  //验证发生变化的值是否是标准输入端
                str_len=read(0,buf,BUF_SIZE); 
                buf[str_len]=0;
                printf("Message from console: %s",buf);
            }
        }
    }
    return 0;
}