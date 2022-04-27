#include<stdio.h>

#define BUF_SIZE 100

//标准io实现文件复制

int main(int argc,char *argv[]){
    FILE *fp1;
    FILE *fp2;
    char buf[BUF_SIZE];
    int len;

    fp1=fopen("news.txt","r");
    fp2=fopen("cpy2.txt","w");

    while(fgets(buf,BUF_SIZE,fp1)!=NULL){
        fputs(buf,fp2);
    }

    fclose(fp1);
    fclose(fp2);

    return 0;
}