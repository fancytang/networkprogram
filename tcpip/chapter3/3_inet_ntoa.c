#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>

//将ip从结构体中 32int 转换为 string

int main(int argc,char *argv[]){
    struct sockaddr_in addr1,addr2;
    //保存提取出来的string 
    char *str_ptr;
    //存放 提取出来的string
    char str_arr[20];

    //先构造要用的结构体
    addr1.sin_addr.s_addr=htonl(0x1020304);
    addr2.sin_addr.s_addr=htonl(0x1010101);

    //把结构体的int 32 提取成 string
    str_ptr=inet_ntoa(addr1.sin_addr);
    strcpy(str_arr,str_ptr);
    printf("Dotted-Decimal nation1: %s \n",str_ptr);

    inet_ntoa(addr2.sin_addr);
    //覆盖掉之前的内容
    printf("Dotted-Decimal nation2: %s \n",str_ptr);
    printf("Dotted-Decimal nation3: %s \n",str_arr);

    return 0;
}
