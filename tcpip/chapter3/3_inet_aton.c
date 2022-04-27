#include<stdio.h>
// #include<string>
#include<stdlib.h>
#include<arpa/inet.h>

//将ip从string 转换为 32 int
//直接放到sockaddr_in 结构体中

void errorHandling(const char* message);

int main(int argc,char *argv[]){
    char *addr="127.232.124.79";

    struct sockaddr_in addr_inet;

     //将ip从string 转换为 32 int，并直接放到结构体中。<arpa/inet.h>
    if(!inet_aton(addr,&addr_inet.sin_addr)){
        errorHandling("Conversion error.\n");
    }else{
        printf("Network ordered integer addr: %#x\n",addr_inet.sin_addr.s_addr);
    }

    return 0;
}

void errorHandling(const char* message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}