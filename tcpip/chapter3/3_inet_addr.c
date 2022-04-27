#include<stdio.h>
#include<arpa/inet.h>

//将ip从string 转换为 32 int
//同时还能进行网络字节序的转换 和 ip合法验证

int main(int argc,char *argv[]){
    char *addr1="1.2.3.4";
    char *addr2="1.2.3.256";

    //将ip从string 转换为 32 int  <arpa/inet.h>
    unsigned long conv_addr=inet_addr(addr1);
    if(conv_addr==INADDR_NONE){
        printf("Error occured!\n");
    }else{
        printf("Network ordered integer address : %#lx \n", conv_addr);
    }

    conv_addr=inet_addr(addr2);
    if(conv_addr==INADDR_NONE){
        printf("Error occured!\n");
    }else{
        printf("Network ordered integer address : %#lx \n", conv_addr);
    }

    return 0;
}