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
    FILE *fp1,*fp2;
    char buf[BUF_SIZE];

    //打开文件1读取数据
    /**
    //FILE *fopen(const char *filename, const char *mode)
    //该函数返回一个 FILE 指针。否则返回 NULL，且设置全局变量 errno 来标识错误
    **/
    fp1=fopen("1_data.txt","r");
    if(fp1==NULL){
        error_handling("fp1 open() error!");
    }

    //打开文件2写入数据，实现复制
    fp2=fopen("1_data_copy_ansi.txt","w");
    if(fp2==NULL){
        error_handling("fp2 open() error!");
    }
    /**
    char *fgets(char *str, int n, FILE *stream),返回一个空指针。
    **/
    while(fgets(buf,BUF_SIZE,fp1)!=NULL){
        //int fputs(const char *str, FILE *stream)
        //该函数返回一个非负值，如果发生错误则返回 EOF。
        if(fputs(buf,fp2)==EOF){
            error_handling("write() error!");
        }
    }
    
    fclose(fp1);
    fclose(fp2);

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