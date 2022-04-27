#include<stdio.h>
#include<unistd.h>

int main(void){
    int fd1,fd2;
    char str1[]="Hi!\n";
    char str2[]="It's a nice day!\n";

    fd1=dup(1);  //复制后的fd
    fd2=dup2(fd1,7); //复制后的fd

    printf("fd1=%d,fd2=%d \n",fd1,fd2);
    write(fd1,str1,sizeof(str1));
    write(fd2,str2,sizeof(str2));

    close(fd1); //删除复制后的fd
    close(fd2); //删除复制后的fd

    write(1,str1,sizeof(str1));
    close(1);  //删除原fd
    write(1,str1,sizeof(str1));
}