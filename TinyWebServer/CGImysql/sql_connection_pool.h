#ifndef __SQL_CONNECTION_POOL
#define __SQL_CONNECTION_POOL

#include<stdio.h>
#include "../lock/locker.h"
#include<list>
#include<string.h>
#include<string>
#include<mysql/mysql.h>

using namespace std;

class connection_pool{
public:
    //局部静态变量单例模式
    static connection_pool *GetInstance();

    void init(string Url,string User,string Password,string DatabaseName,int Port,int MaxConn);

    MYSQL *GetConnection();  //获取数据库连接

    bool ReleaseConnection(MYSQL *conn);  //释放连接

    int GetFreeConn();

    void DestroyPool();

private:
    connection_pool();
    ~connection_pool();

private:
    unsigned int m_MaxConn; //最大连接数
    unsigned int m_CurConn;  //当前已使用的连接数
    unsigned int m_FreeConn; //当前空闲的连接数

    locker lock;  //对连接池上锁
    list<MYSQL *> connList;  //连接池
    sem reverse;  //信号量 连接池的连接数

private:
    string m_Url;   //主机地址
    string m_Port;  //数据库端口号
    string m_User;  //登陆数据库用户名
    string m_Password;   //登陆数据库密码
    string m_Database_Name;  //使用数据库名
};

class connectionRAII{
public:
    connectionRAII(MYSQL **con,connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};

#endif
