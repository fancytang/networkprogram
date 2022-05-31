#ifndef __HTTP_CONN_H
#define __HTTP_CONN_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<assert.h>
#include<sys/mman.h>  //内存映射
#include<errno.h>
#include<stdarg.h>  //可变参数
#include<sys/uio.h>


#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<signal.h>
#include<sys/stat.h>
#include<sys/epoll.h>
#include<pthread.h>

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"
#include "../threadpool/threadpool.h"

class http_conn{
public:
    static const int FILENAME_LEN=200;  //文件名size
    static const int WRITE_BUFFER_SIZE=1024;//write buf size
    static const int READ_BUFFER_SIZE=2048;//read buf size

    enum METHOD{
        GET=0,
        HEAD,
        POST,
        PUT,
        DELETE,  //删除
        TRACE,  //回显
        OPTIONS,  //允许查看server性能
        CONNECT,
        PATH
    };

    //表示解析位置
    //主状态机的处理状态:请求行 请求头 消息体(仅解析POST请求)
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE=0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT  
    };

    //报文解析结果
    enum HTTP_CODE{
        NO_REQUEST,
        GET_REQUEST,   //请求成功 200
        BAD_REQUEST,   // 客户端请求的语法错误，服务器无法理解 400
        NO_RESOURCE,  //服务器无法根据客户端的请求找到资源   404
        FORBIDDEN_REQUEST,  //服务器理解请求客户端的请求，但是拒绝执行此请求 403
        FILE_REQUEST,
        INTERNAL_ERROR,  //服务器内部错误 500
        CLOSED_CONNECTION
    };

    //从状态机:标识每一行的解析状态
    enum LINE_STATUS{
        LINE_OK=0,  //完整读取一行
        LINE_BAD,   //报文语法有误
        LINE_OPEN //读取行不完整
    };

public:
    http_conn(){}
    ~http_conn(){}
public:
    void init(int sockfd,const sockaddr_in &addr);

    //读取浏览器发来的消息
    bool read_once();

    //处理报文
    void process();

    //发出响应报文
    bool write();

    //关闭http连接
    void close_conn(bool real_close=true);

    sockaddr_in *get_address(){ return &m_address;}

    //初始化数据库表
    void initmysql_result(connection_pool *connPool);

private:
    void init();

    HTTP_CODE process_read();

    bool process_write(HTTP_CODE ret);

    HTTP_CODE do_request();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_request_headers(char *text);
    HTTP_CODE parse_request_content(char *text);

    char *get_line(){
        return m_read_buf+m_start_line;
    }

    LINE_STATUS parse_line();

    void unmap();

    bool add_response(const char *format,...);
    bool add_content(const char *content);
    bool add_status_line(int status,const char* title);
    bool add_headers(int content_len);
    bool add_content_type();
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;

private:
    //和socket有关
    int m_sockfd;
    sockaddr_in m_address;

    //和message(buffer)有关
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;  //m_read_buf data 最后一个字节的下一个位置
    int m_checked_idx; //m_read_buf读取位置
    int m_start_line;  //m_read_buf已解析位置

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx; //buffer长度

    //和http解析有关
    CHECK_STATE m_check_state; //解析状态
    METHOD m_method; //请求方法
    char *m_string;//请求消息

    char m_real_file[FILENAME_LEN];  //请求的文件名
    char *m_file_address;  //请求的文件地址
    struct stat m_file_stat;  //请求的文件状态

    char *m_url;    //url
    char *m_version;    //http版本
    char *m_host;   //主机
    int m_content_length;   //消息体长度
    bool m_linger;  //是否保持http连接
    
    int m_cgi;//是否启用的POST ,即html中的action,具体行为动作

    //读取地址
    struct iovec m_iv[2];
    int m_iv_count;

    //发送message
    int bytes_to_send;
    int bytes_have_send;

};


#endif

