#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>

#define BUF_SIZE 100

//将fd转为FILE struct

int main(int argc,char *argv[]){
    FILE *fp;
    int fd=open("news.txt",O_WRONLY|O_CREAT|O_TRUNC);
    if(fd==-1){
        fputs("file open error",stdout);
    }

    fp=fdopen(fd,"w");
    fputs("Network C programming \n",fp);

    fclose(fp);
    close(fd);
}