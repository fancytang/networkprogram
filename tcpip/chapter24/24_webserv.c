#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

#define BUF_SIZE 1024
#define SMALL_BUF 100

void* request_handler(void* arg);
void send_data(FILE *fp,char *ct,char *file_name);
void send_error(FILE *fp);
char* content_type(char* file);
void error_handling(char *msg);

int main(int argc,char *argv[]){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    int clnt_addr_size;

    char buf[BUF_SIZE];

    pthread_t t_id;

    if(argc!=2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock=socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        error_handling("bind() error");
    }
    if(listen(serv_sock,20)==-1){
        error_handling("listen() error");
    }

    while(1){
        clnt_addr_size=sizeof(clnt_addr);
        clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&clnt_addr_size);
        printf("Connect Request : %s:%d \n",inet_ntoa(clnt_addr.sin_addr),ntohs(clnt_addr.sin_port));

        pthread_create(&t_id,NULL,request_handler,&clnt_sock);
        pthread_detach(t_id);
    }
    
    close(serv_sock);
    return 0;
}

void* request_handler(void* arg){
    int clnt_sock=*((int*)arg);
    FILE *clnt_read,*clnt_write;

    char req_line[SMALL_BUF];
    char method[10];
    char ct[15];  //内容类型 html或plain
    char file_name[30];

    clnt_read=fdopen(clnt_sock,"r");
    clnt_write=fdopen(dup(clnt_sock),"w");  //复制fd

    fgets(req_line,SMALL_BUF,clnt_read);

    //是不是HTTP请求
    //C 库函数 char *strstr(const char *haystack, const char *needle) 在字符串 haystack 中查找第一次出现字符串 needle 的位置，不包含终止符 '\0'。
    if(strstr(req_line,"HTTP/")==NULL){
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }

    //C 库函数 char *strtok(char *str, const char *delim) 分解字符串 str 为一组字符串，delim 为分隔符。
    strcpy(method,strtok(req_line," /"));
    strcpy(file_name,strtok(NULL," /"));
    strcpy(ct,content_type(file_name));
    
    //请求方式是不是GET 
    if(strcmp(method,"GET")!=0){
        send_error(clnt_write);
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }

    fclose(clnt_read);
    send_data(clnt_write,ct,file_name);
}

void send_data(FILE *fp,char *ct,char *file_name){
    char ptotocol[]="HTTP/1.0 200 OK\r\n";
    char server[]="Server:Linux Web Server \r\n";
    char cnt_len[]="Content-length:2048\r\n";
    char cnt_type[SMALL_BUF];
    char buf[BUF_SIZE];
    FILE *send_file;

    sprintf(cnt_type,"Content-type:%s\r\n\r\n",ct);
    send_file=fopen(file_name,"r");
    if(send_file==NULL){
        send_error(fp);
        return;
    }

    //传输头信息
    fputs(ptotocol,fp);
    fputs(server,fp);
    fputs(cnt_len,fp);
    fputs(cnt_type,fp);

    //传输请求数据
    while(fgets(buf,BUF_SIZE,send_file)!=NULL){
        fputs(buf,fp);
        fflush(fp);
    }

    fflush(fp);
    fclose(fp);  

}

void send_error(FILE *fp){
    char ptotocol[]="HTTP/1.0 400 Bad Request\r\n";
    char server[]="Server:Linux Web Server \r\n";
    char cnt_len[]="Content-length:2048\r\n";
    char cnt_type[]="Content-type:text/html\r\n\r\n";
    char content[]="<html><head><title>NETWORK</title></head>"
                     "<body><font size=+5><br>发生错误！ 查看请求文件名和请求方式!"
                     "</font></body></html>";
    fputs(ptotocol,fp);
    fputs(server,fp);
    fputs(cnt_len,fp);
    fputs(cnt_type,fp);
    //fputs(content,fp);
    fflush(fp);
}

//find content_type: html/plain
char* content_type(char* file){
    char file_name[SMALL_BUF];
    char extension[SMALL_BUF];

    strcpy(file_name,file);
    strtok(file_name,".");
    
    //NULL的作用只是为了使得每次调用时，都不是从头开始，而是从上次调用时查找所停止的位置开始，
    strcpy(extension,strtok(NULL,"."));

    if(!strcmp(extension,"html")||!strcmp(extension,"htm")){
        return "text/html";
    }else{
        return "text/plain";
    }
}


void error_handling(char *msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}
