#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

//系统io实现文件复制

#define BUF_SIZE 100

int main(int argc,char *argv[]){
    int fd1,fd2;
    char buf[BUF_SIZE];
    int len;

    fd1=open("news.txt",O_RDONLY);
    fd2=open("cpy.txt",O_WRONLY|O_CREAT|O_TRUNC);

    while((len=read(fd1,buf,sizeof(buf)))>0){
        write(fd2,buf,len);
    }

    close(fd1);
    close(fd2);

    return 0;
}