#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/socket.h>

//创建套接字和文件 测试其描述符

// 0 1 2 是分配给标准IO的描述符
int main(void){
    int fd1,fd2,fd3;

    //<sys/socket.h>
    fd1=socket(PF_INET,SOCK_STREAM,0);
    fd2=open("1_test.txt",O_CREAT|O_WRONLY|O_TRUNC);
    fd3=socket(PF_INET,SOCK_DGRAM,0);

    printf("file descriptor 1: %d\n",fd1);
    printf("file descriptor 2: %d\n",fd2);
    printf("file descriptor 3: %d\n",fd3);

    close(fd1);
    close(fd2);
    close(fd3);

    return 0;
}