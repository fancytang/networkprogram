#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<unistd.h>

#define BUF_SIZE 100

//低级IO实现文件复制

/**
<unistd.h> 是 C 和 C++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件的名称。
该头文件由 POSIX.1 标准（可移植系统接口）提出.
write() close()要用到

fcntl.h，是unix标准中通用的头文件，其中包含的相关函数有 open，fcntl，shutdown，unlink，fclose等！
open中的flag、open要用到
**/

void error_handling(char* message);

int main(void){
    int fd1,fd2;
    char buf[BUF_SIZE];

    //打开文件1读取数据
    fd1=open("1_data.txt",O_RDONLY);
    if(fd1==-1){
        error_handling("fd1 open() error!");
    }
    if(read(fd1,buf,sizeof(buf))==-1){
        error_handling("fd1 read() error!");
    }

    //打开文件2写入数据，实现复制
    fd2=open("1_data_copy.txt",O_CREAT|O_WRONLY|O_TRUNC);
    if(fd2==-1){
        error_handling("fd2 open() error!");
    }
    if(write(fd2,buf,sizeof(buf))==-1){
        error_handling("fd2 write() error!");
    }

    close(fd1);
    close(fd2);

    //EXIT_SUCCESS和EXIT_FAILURE符号常量，作为exit（）的参数来使用，分别表示成功和没有成功的执行一个程序
    return EXIT_SUCCESS;
}

void error_handling(char* message){
    //<stdio.h>
    fputs(message, stderr);
    fputc('\n', stderr);
    //退出程序，并将参数value的值返回给主调进程<stdlib.h>
    exit(1);
}