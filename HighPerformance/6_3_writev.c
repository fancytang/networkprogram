#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>   //c没有bool

#define BUF_SIZE 1024
static const char *status_line[2]={"200 OK","500 Internal server error"};
/*定义两种HTTP状态码和状态信息*/


int main(int argc, const char *argv[]){
    if(argc != 3){
        printf("Usage: %s <port> <filename> \n", argv[1]);
        exit(1);
    }

    const char* file_name=argv[2];

    int sockServ = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockServ>=0);

    struct sockaddr_in sockServAddr;
    memset(&sockServAddr, 0, sizeof(sockServAddr));
    sockServAddr.sin_family = AF_INET;
    sockServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockServAddr.sin_port = htons(atoi(argv[1]));

    int ret=bind(sockServ, (struct sockaddr*)& sockServAddr, sizeof(sockServAddr));
    assert(ret!=-1);

    ret=listen(sockServ, 5);
    assert(ret!=-1);

    struct sockaddr_in sockClientAddr;
    socklen_t clientAddrLen = sizeof(sockClientAddr);

    int sockClient = accept(sockServ, (struct sockaddr*)& sockClientAddr, &clientAddrLen);

    if(sockClient<0){
        //char *strerror(int errnum)返回一个指向错误字符串的指针 C 标准库 - <string.h>
        fprintf(stderr,"accept error:errno:%d,strerr:%s \n ",errno,strerror(errno));
    }else{
        /*用于保存HTTP应答的状态行、头部字段和一个空行的缓存区*/
        char header_buf[BUF_SIZE];
        memset(&header_buf,0,sizeof(header_buf));

        /*用于存放目标文件内容的应该用程序缓存*/
        char* file_buf=NULL;

        /*用于获取目标文件的属性，比如是否为目录， 文件大小等*/
        struct stat file_stat;

        /*记录目标文件是否是有效文件*/
        bool valid = true;

        int len=0;

        //int stat(const char *path, struct stat *buf)返回值：成功返回0，失败返回-1；
        if(stat(file_name,&file_stat)<0){   /*目标文件不存在*/
            valid=false;
        }else{
            if(S_ISDIR(file_stat.st_mode)){ //目标文件是一个目录
                valid=false;
            }else if(file_stat.st_mode&S_IROTH){  /*当前用户有读取目标文件爱你的权限*/
                /*动态分配缓存区file_buf,
                 * 并指定其大小为目标的大小file_stat.st_size加1,
                 * 然后将目标文件读入缓存区file_buf中*/
                int fd=open(file_name,O_RDONLY);
                // file_buf=new char[file_stat.st_size+1];
                file_buf=malloc(sizeof(char)*file_stat.st_size);
                memset(file_buf, '\0', file_stat.st_size + 1);
                if(read(fd,file_buf,file_stat.st_size)<0){
                    valid=false;
                }
            }else{
                valid=false;
            }
        }

        /*如果目标文件有效， 则发送正常的HTTP应答 */
        if(valid){
            ret=snprintf(header_buf,BUF_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[0]);
            len+=ret;
            ret=snprintf(header_buf+len,BUF_SIZE-1-len,"Content-Length: %lld\r\n", (long long)file_stat.st_size);

            /*利用writev将header_buf和file_buf的内容一并写出 */
            struct iovec iv[2];
            iv[0].iov_base=header_buf;
            iv[0].iov_len=strlen(header_buf);
            iv[1].iov_base=file_buf;
            iv[1].iov_len=file_stat.st_size;

            ret=writev(sockClient,iv,2);
        }else{  /*如果目标文件无效， 则通知客户端服务器发生了“内部错误”*/
            ret=snprintf(header_buf,BUF_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[1]);
            len+=ret;
            ret=snprintf(header_buf+len,BUF_SIZE-1-len,"Content-Length: %lld\r\n", (long long)file_stat.st_size);

            write(sockClient,header_buf,strlen(header_buf));
        }

        close(sockClient);
        if(file_buf){  //没有分配空间就不需要释放
            free(file_buf);
        }
    }
    close(sockServ);
    return 0;
}
