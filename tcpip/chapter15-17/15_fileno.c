#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

#define BUF_SIZE 100

//将FILE struct转为 fd

int main(int argc,char *argv[]){
    FILE *fp;
    int fd=open("news.txt",O_WRONLY|O_CREAT|O_TRUNC);
    if(fd==-1){
        fputs("file open error",stdout);
    }

    printf("First fd : %d \n",fd);
    fp=fdopen(fd,"w");
    fputs("TCP/IP SOCKET programming \n",fp);
    printf("Second fd : %d \n",fileno(fp));

    close(fd);
}