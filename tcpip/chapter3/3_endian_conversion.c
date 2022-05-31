#include<stdio.h>
#include<arpa/inet.h>

//网络传输时要先将数据转换为网络字节序

int main(int argc,char *argv[]){
    //主机端口号和网络端口号 short 2bytes
    unsigned short host_port=0x1234;
    unsigned short net_port;

    //主机ip 和 网络ip long 4bytes
    unsigned long host_addr=0x12345678;
    unsigned long net_addr;

    //主机向外发送
    // 转换为endian bytes  <arpa/inet.h>
    net_port=htons(host_port);
    net_addr=htonl(host_addr);

    printf("Host ordered port: %#x \n",host_port);
    printf("Network ordered port: %#x \n",net_port);
    printf("Host ordered address: %#lx \n",host_addr);
    printf("Network ordered address: %#lx \n",net_addr);

    return 0;
}