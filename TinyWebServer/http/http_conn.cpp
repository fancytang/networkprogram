#include "./http_conn.h"
#include "../log/log.h"

#include<map>
#include<fstream>
#include<iostream>
using namespace std;

// #define connfdET 
#define connfdLT

// #define listenfdET
#define listenfdLT

const char *ok_200_title="OK";
const char *error_400_title="Bad Request";
const char *error_400_form= "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title="Forbidden";
const char *error_403_form="You do not have permission to get file form this server.\n";
const char *error_404_title="Not found";
const char *error_404_form="The requested file was not found on this server.\n";
const char *error_500_title="Internal Error";
const char *error_500_form="There was an unusual problem serving the request file.\n";

int setnonblocking(int fd){
    int old_opt=fcntl(fd,F_GETFL);
    int new_opt=old_opt|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_opt);
    return old_opt;
}

void addfd(int epollfd,int fd,bool one_shot){
    epoll_event event;
    event.data.fd=fd;

#ifdef connfdET
    event.events=EPOLLIN|EPOLLET|EPOLLRDHUP;
#endif
#ifdef connfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
    event.events=EPOLLIN|EPOLLET|EPOLLRDHUP;
#endif
#ifdef listenfdLT
    event.events=EPOLLIN|EPOLLRDHUP;
#endif

    if(one_shot){
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void removefd(int epollfd,int fd){
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
//ev为EPOLLIN EPOLLOUT
void modfd(int epollfd,int fd,int ev){
    epoll_event event;
    event.data.fd=fd;

#ifdef connfdET
    event.events=ev|EPOLLET|EPOLLRDHUP|EPOLLONESHOT;
#endif
#ifdef connfdLT
    event.events = ev | EPOLLRDHUP|EPOLLONESHOT;
#endif
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

int http_conn::m_epollfd=-1;
int http_conn::m_user_count=0;

//将表中的用户名和密码放入users
map<string,string> users;
locker m_lock; //数据库的锁
//初始化数据库表
void http_conn::initmysql_result(connection_pool *connPool){
    MYSQL *mysql=NULL;
    connectionRAII mysqlcon(&mysql,connPool);

    if(mysql_query(mysql,"SELECT username,passwd FROM user")){
        LOG_ERROR("SELECT error:%s\n",mysql_error(mysql));
    }
    //从表中检索完整的结果集
    MYSQL_RES *result=mysql_store_result(mysql);
    //返回结果集中的列数
    int num_fields=mysql_num_fields(result);
    //返回所有字段结构的数组
    MYSQL_FIELD *fields=mysql_fetch_fields(result);

    while(MYSQL_ROW row=mysql_fetch_row(result)){
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1]=temp2;
    }

}

//关闭http连接,关闭一个连接，客户总量减一
void http_conn::close_conn(bool real_close){
    if(real_close&&(m_sockfd!=-1)){
        removefd(m_epollfd,m_sockfd);
        m_user_count--;
        m_sockfd=-1;
    }
}


void http_conn::init(int sockfd,const sockaddr_in &addr){
    m_sockfd=sockfd;
    m_address=addr;

    addfd(m_epollfd,m_sockfd,true);
    m_user_count++;
    init();
}
void http_conn::init(){
    mysql=NULL;

    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    memset(m_write_buf,'\0',WRITE_BUFFER_SIZE);
    memset(m_real_file,'\0',FILENAME_LEN);

    m_read_idx=0;
    m_checked_idx=0;
    m_start_line=0;
    m_write_idx=0;

    m_check_state=CHECK_STATE_REQUESTLINE;
    m_method=GET;

    m_string=0;
    // m_file_address=0;  ??
    m_url=0;
    m_version=0;
    m_host=0;
    m_content_length=0;
    m_linger=false;

    m_cgi=0;

    bytes_to_send=0;
    bytes_have_send=0;
}

//处理报文
void http_conn::process(){
    HTTP_CODE read_ret=process_read();

    //请求不完整,需要继续接收数据
    if(read_ret==NO_REQUEST){
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return;
    }

    //回应,根据read_ret有不同回应
    bool write_ret=process_write(read_ret);
    if(!write_ret){
        close_conn();
    }
    //注册并监听写事件
    modfd(m_epollfd,m_sockfd,EPOLLOUT);
}

//对读入的报文进行解析
http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATUS line_status=LINE_OK;
    //行解析结果
    HTTP_CODE ret=NO_REQUEST;
    //本次解析的消息
    char *text=0;

    //循环要能接收get和post post消息体末尾没有字符,通过主状态机进行
    //get 每行都有\r\n,通过从状态机
    while((m_check_state==CHECK_STATE_CONTENT&&line_status==LINE_OK)||((line_status=parse_line())==LINE_OK)){
        text=get_line();
        m_start_line=m_checked_idx;
        LOG_INFO("%s",text);
        Log::get_instance()->flush();
        //主状态机的逻辑转移
        switch(m_check_state){
            case CHECK_STATE_REQUESTLINE:
            {
                ret=parse_request_line(text);
                if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret=parse_request_headers(text);
                if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }
                //get 跳转到报文相应函数
                else if(ret==GET_REQUEST){
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret=parse_request_content(text);
                if(ret==GET_REQUEST){
                    return do_request();
                }
                //解析完消息体后避免再次进入循环,将行状态改为OPEN
                line_status=LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

//从状态机,解析出一行内容,然后传给主状态机分析是什么
http_conn::LINE_STATUS http_conn::parse_line(){
    char temp;
    for(;m_checked_idx<m_read_idx;m_checked_idx++){
        temp=m_read_buf[m_checked_idx];
        if(temp=='\r'){
            //读到行不完整
            if(m_checked_idx+1==m_read_idx){
                return LINE_OPEN;
            }
            //读到完整行,将\r\n替换为\0\0,供主状态机直接读出处理
            else if(m_read_buf[m_checked_idx+1]=='\n'){
                m_read_buf[m_checked_idx++]='\0';
                m_read_buf[m_checked_idx++]='\0';
                return LINE_OK;
            }
            //都不符合,BAD
            return LINE_BAD;
        }else if(temp=='\n'){
            if(m_checked_idx>1&&m_read_buf[m_checked_idx-1]=='\r'){
                m_read_buf[m_checked_idx-1]='\0';
                m_read_buf[m_checked_idx++]='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
    //先解析请求行,包含请求类型get/post 资源 http协议版本
    //先找出第一个空格
    m_url=strpbrk(text," \t");
    if(!m_url){
        return BAD_REQUEST;
    }
    *m_url++='\0';

    //确定请求方式
    char *method=text;
    if(strcasecmp(method,"GET")==0){
        m_method=GET;
    }else if(strcasecmp(method,"POST")==0){
        m_method=POST;
        m_cgi=1;
    }else{
        return BAD_REQUEST;
    }

    //跳过空格
    m_url+=strspn(m_url," \t");
    
    //取出版本
    m_version=strpbrk(m_url," \t");
    if(!m_version){
        return BAD_REQUEST;
    }
    *m_version++='\0';
    m_version+=strspn(m_version," \t");

    if(strcasecmp(m_version,"HTTP/1.1")!=0){
        return BAD_REQUEST;
    }   
    //去掉http://
    if(strncasecmp(m_url,"http://",7)==0){
        m_url+=7;
        m_url=strchr(m_url,'/');
    }
    //去掉https://
    if(strncasecmp(m_url,"https://",8)==0){
        m_url+=8;
        m_url=strchr(m_url,'/');
    }
    //直接是访问资源 /XXX.html
    if(!m_url||m_url[0]!='/'){
        return BAD_REQUEST;
    }

    if(strlen(m_url)==1){
        strcat(m_url,"judge.html");
    }

    m_check_state=CHECK_STATE_HEADER;
    return NO_REQUEST;  
    
}

//解析请求头和空行
//如果是空行，判断content-length，为0是GET，否则为POST
http_conn::HTTP_CODE http_conn::parse_request_headers(char *text){
    if(text[0]=='\0'){
        if(m_content_length!=0){
            //POST 跳转到解析消息体
            m_check_state=CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }else if(strncasecmp(text,"Connection:",11)==0){
        //Connection: Keep-Alive
        text+=11;

        text+=strspn(text," \t");
        if(strcasecmp(text,"keep-alive")==0){
            //长连接
            m_linger=true;
        }

    }else if(strncasecmp(text,"Content-length:",15)==0){
        text+=15;

        text+=strspn(text," \t");
        m_content_length=atol(text);
    }else if(strncasecmp(text,"Host:",5)==0){
        text+=5;
        text+=strspn(text," \t");
        m_host=text;
    }else{
        // printf("oop!unknown header: %s\n",text);
        LOG_INFO("oop!unknown header: %s\n",text);
        Log::get_instance()->flush();
    }
    return NO_REQUEST;
}
http_conn::HTTP_CODE http_conn::parse_request_content(char *text){
    if((m_content_length+m_checked_idx)<=m_read_idx){
        text[m_content_length]='\0';

        m_string=text;

        return GET_REQUEST;
    }
    return NO_REQUEST;
}

//读取浏览器发来的消息
bool http_conn::read_once(){
    if(m_read_idx>=READ_BUFFER_SIZE){
        return false;
    }
    int bytes_read=0;

#ifdef connfdET
    while(1){
        bytes_read=recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
    
        if(bytes_read==-1){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }   
            return false;
        }

        else if(bytes_read==0){
            return false;
        }

        m_read_idx+=bytes_read;

    }
    return true;
#endif

#ifdef connfdLT
    bytes_read=recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
    m_read_idx+=bytes_read;
    
    if(bytes_read<=0){
        return false;
    }

    return true;

#endif

}

//发出响应报文
bool http_conn::write(){
    if(bytes_to_send==0){
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        init();
        return true;
    }
    int temp=0;
    while(1){
        temp=writev(m_sockfd,m_iv,m_iv_count);
        if(temp<0){
            if(errno==EAGAIN){
                modfd(m_epollfd,m_sockfd,EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send+=temp;
        bytes_to_send-=temp;

        if(bytes_have_send>=m_iv[0].iov_len){
            m_iv[0].iov_len=0;
            m_iv[1].iov_base=m_file_address+(bytes_have_send-m_write_idx);
            m_iv[1].iov_len=bytes_to_send;
        }else{
            m_iv[0].iov_base=m_write_buf+bytes_have_send;
            m_iv[0].iov_len=m_iv[0].iov_len-bytes_have_send;
        }

        if(bytes_to_send<=0){
            unmap();
            modfd(m_epollfd,m_sockfd,EPOLLIN);

            if(m_linger){
                init();
                return true;
            }else{
                return false;
            }
        }
    }
    
}


bool http_conn::process_write(HTTP_CODE ret){
    switch(ret){
        case INTERNAL_ERROR: //500内部错误
        {
            add_status_line(500,error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form)){
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(404,error_404_title);
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form)){
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST: //没有访问权限 403
        {
            add_status_line(403,error_403_title);
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form)){
                return false;
            }
            break;
        }
        case FILE_REQUEST:  //文件存在 200
        {
            add_status_line(200,ok_200_title);
            if(m_file_stat.st_size!=0){
                add_headers(m_file_stat.st_size);
                //第一个 指向响应报文缓冲区
                m_iv[0].iov_base=m_write_buf;
                m_iv[0].iov_len=m_write_idx;
                //第二个指向文件地址
                m_iv[1].iov_base=m_file_address;
                m_iv[1].iov_len=m_file_stat.st_size;

                m_iv_count=2;

                bytes_to_send=m_write_idx+m_file_stat.st_size;
                return true;
            }else{
                //请求资源大小为0，返回空白html文件
                const char *ok_string="<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string)){
                    return false;
                }
            }
        }
        default:
            return false;
    }
    //除此之外，只一个指向响应报文缓冲区
    m_iv[0].iov_base=m_write_buf;
    m_iv[0].iov_len=m_write_idx;
    m_iv_count=1;
    bytes_to_send=m_write_idx;
    return true;
}
const char *doc_root = "/home/fancy/study/TinyWebServer/root";
//对请求的文件进行分析
http_conn::HTTP_CODE http_conn::do_request(){
    //网站根目录
    strcpy(m_real_file,doc_root);
    int len=strlen(doc_root);
    //找到url中/的位置
    const char *p=strrchr(m_url,'/');

    //处理cgi
    if(m_cgi==1&&(*(p+1)=='2'||*(p+1)=='3')){
        //根据标志判断是登录检测还是注册检测
        char flag=m_url[1];

        char *m_url_real=(char*)malloc(sizeof(char)*200);
        strcpy(m_url_real,"/");
        strcat(m_url_real,m_url+2);
        strncpy(m_real_file+len,m_url_real,FILENAME_LEN-len-1);
        free(m_url_real);

        //提取用户名和密码
        //user=123&passwd=123
        char name[100],password[100];
        int i=0;
        for(i=5;m_string[i]!='&';i++){
            name[i-5]=m_string[i];
        }
        name[i-5]='\0';

        int j=0;
        for(i=i+10;m_string[i]!='\0';i++,j++){
            password[j]=m_string[i];
        }
        password[j]='\0';

        //同步线程登录校验
        if(*(p+1)=='3'){
            //注册，先检测db是否有重名的
            char *sql_insert=(char*)malloc(sizeof(char)*200);
            strcpy(sql_insert,"INSERT INTO user(username,passwd) VALUES(");
            strcat(sql_insert,"'");
            strcat(sql_insert,name);
            strcat(sql_insert,"', '");
            strcat(sql_insert,password);
            strcat(sql_insert,"')");

            if(users.find(name)==users.end()){
                m_lock.lock();
                int res=mysql_query(mysql,sql_insert);
                users.insert(pair<string,string>(name,password));
                m_lock.unlock();

                if(!res){
                    strcpy(m_url,"/login.html");
                }else{
                    strcpy(m_url,"/registerErr.html");
                }
            }else{
                strcpy(m_url,"/registerErr.html");
            }
        }
        //如果是直接登录
        else if(*(p+1)=='2'){
            if(users.find(name)!=users.end()&&users[name]==password){
                strcpy(m_url,"/welcome.html");
            }else{
                strcpy(m_url,"/loginErr.html");
            }
        }

        //CGI多进程登录校验
    }
    // /0 注册界面
    if(*(p+1)=='0'){
        char *m_url_real=(char*)malloc(sizeof(char)*200);
        strcpy(m_url_real,"/register.html");

        strncpy(m_real_file+len,m_url_real,strlen(m_url_real));

        free(m_url_real);
    }
    //  /1 登录界面
    else if(*(p+1)=='1'){
        char *m_url_real=(char*)malloc(sizeof(char)*200);
        strcpy(m_url_real,"/login.html");

        strncpy(m_real_file+len,m_url_real,strlen(m_url_real));

        free(m_url_real);
    }
    //  /1 请求图片界面
    else if(*(p+1)=='5'){
        char *m_url_real=(char*)malloc(sizeof(char)*200);
        strcpy(m_url_real,"/showpic.html");

        strncpy(m_real_file+len,m_url_real,strlen(m_url_real));

        free(m_url_real);
    }
    //  /1 请求视频界面
    else if(*(p+1)=='6'){
        char *m_url_real=(char*)malloc(sizeof(char)*200);
        strcpy(m_url_real,"/showvid.html");

        strncpy(m_real_file+len,m_url_real,strlen(m_url_real));

        free(m_url_real);
    }
    // 都不是 welcome界面
    else{
        strncpy(m_real_file+len,m_url,FILENAME_LEN-len-1);
    }

    //先检查有没有这个资源
    if(stat(m_real_file,&m_file_stat)<0){
        return NO_RESOURCE;
    }
    //在检查有没有权限
    if(!(m_file_stat.st_mode&S_IROTH)){
        return FORBIDDEN_REQUEST;
    }
    //检查是不是目录
    if(S_ISDIR(m_file_stat.st_mode)){
        return BAD_REQUEST;
    }
    //都不是，可正常获取资源
    int fd=open(m_real_file,O_RDONLY);
    //将文件映射到内存
    m_file_address=(char*)mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);

    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap(){
    if(m_file_address){
        munmap(m_file_address,m_file_stat.st_size);
        m_file_address=0;
    }
}

bool http_conn::add_response(const char *format,...){
    if(m_write_idx>=WRITE_BUFFER_SIZE){
        return false;
    }
    //定义可变参数列表
    va_list arg_list;
    //将变量arg_list初始化为传入参数
    va_start(arg_list,format);
    //将数据format从可变参数列表写入缓冲区，返回长度
    int len=vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,arg_list);

    if(len>=(WRITE_BUFFER_SIZE-1-m_write_idx)){
        va_end(arg_list);
        return false;
    }

    m_write_idx+=len;
    //清空可变参列表
    va_end(arg_list);

    LOG_INFO("request:%s",m_write_buf);
    Log::get_instance()->flush();

    return true;
    
}

//状态行 HTTP/1.1 200 OK
bool http_conn::add_status_line(int status,const char* title){
    return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}
//消息报头
bool http_conn::add_headers(int content_len){
    add_content_length(content_len);
    add_linger();
    add_blank_line();
}
bool http_conn::add_content(const char *content){
    return add_response("%s",content);
}
bool http_conn::add_content_type(){
    return add_response("Content-Type:%s\r\n","text/html");
}
bool http_conn::add_content_length(int content_len){
    return add_response("Content-Length:%d\r\n",content_len);
}
bool http_conn::add_linger(){
    return add_response("Connection:%s\r\n",(m_linger==true)?"keep-alive":"close");
}
bool http_conn::add_blank_line(){
    return add_response("%s","\r\n");
}
