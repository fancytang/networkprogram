#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<unistd.h>

//打开只读文件读取数据，低级IO方式

/**
<unistd.h> 是 C 和 C++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件的名称。
该头文件由 POSIX.1 标准（可移植系统接口）提出.
write() 要用到

fcntl.h，是unix标准中通用的头文件，其中包含的相关函数有 open，fcntl，shutdown，unlink，fclose等！
open中的flag、open要用到
**/

#define BUF_SIZE 100

void error_handling(char* message);

int main(void){
    int fd;
    char buf[BUF_SIZE];

    //open(path,flag); 打开只读文件 (有权限的root才能打开)
    fd=open("1_data.txt",O_RDONLY);

    if(fd==-1){
        error_handling("open() error!");
    }
    printf("file descriptor: %d\n",fd);

    //read(fd,buf,size);
    if(read(fd,buf,sizeof(buf))==-1){
        error_handling("read() error!");
    }
    printf("file data: %s \n",buf);
    close(fd);

    return 0;
}

void error_handling(char* message){
    //<stdio.h>
    fputs(message, stderr);
    fputc('\n', stderr);
    //退出程序，并将参数value的值返回给主调进程<stdlib.h>
    exit(1);
}