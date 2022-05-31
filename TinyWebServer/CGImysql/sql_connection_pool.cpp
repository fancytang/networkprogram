#include "./sql_connection_pool.h"
#include<iostream>
#include<stdlib.h>

using namespace std;

connection_pool::connection_pool(){
    m_CurConn=0;
    m_FreeConn=0;
}

connection_pool::~connection_pool(){
    DestroyPool();
}
 //单例模式
connection_pool *connection_pool::GetInstance(){
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(string Url,string User,string Password,string DatabaseName,int Port,int MaxConn){
    this->m_Url=Url;
    this->m_Port=Port;
    this->m_User=User;
    this->m_Password=Password;
    this->m_Database_Name=DatabaseName;

    lock.lock();

    for(int i=0;i<MaxConn;i++){
        MYSQL *con=NULL;
        con=mysql_init(con);

        if(con==NULL){
            cout<<"Error:"<<mysql_error(con);
            exit(1);
        }

        con=mysql_real_connect(con,Url.c_str(),User.c_str(),Password.c_str(),DatabaseName.c_str(),Port,NULL,0);

        if(con==NULL){
            cout<<"Error:"<<mysql_error(con);
            exit(1);
        }

        //更新连接池和空闲连接数量
        connList.push_back(con);
        m_FreeConn++;

        
    }
    //将信号量初始化为最大连接次数
    reverse=sem(m_FreeConn);

    m_MaxConn=m_FreeConn;

    lock.unlock();
    
}

//当线程数量大于数据库连接数量时，使用信号量进行同步，每次取出连接，信号量原子减1，释放连接原子加1，若连接池内没有连接了，则阻塞等待。

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection(){
    MYSQL *con=NULL;

    if(connList.size()==0){
        return NULL;
    }

    //取出连接，信号量原子减1，为0则等待
    reverse.wait();

    lock.lock();

    con=connList.front();
    connList.pop_front();

    m_FreeConn--;
    m_CurConn++;

    lock.unlock();
    return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *conn){
    if(conn==NULL){
        return false;
    }

    lock.lock();

    connList.push_back(conn);
    m_FreeConn++;
    m_CurConn--;

    lock.unlock();

    //释放连接原子加1
    reverse.post();
    return true;
}

int connection_pool::GetFreeConn(){
    return this->m_FreeConn;
}

//销毁连接池
//通过迭代器遍历连接池链表，关闭对应数据库连接，清空链表并重置空闲连接和现有连接数量。
void connection_pool::DestroyPool(){
    lock.lock();

    if(connList.size()>0){
        list<MYSQL *>::iterator it;

        for(it=connList.begin();it!=connList.end();it++){
            MYSQL *con=*it;
            mysql_close(con);
        }

        m_CurConn=0;
        m_FreeConn=0;

        //清空list
        connList.clear();

        lock.unlock();
    }

    lock.unlock();
}

//不直接调用获取和释放连接的接口，将其封装起来，通过RAII机制进行获取和释放。
//将数据库连接的获取与释放通过RAII机制封装，避免手动释放。
//在获取连接时，通过有参构造对传入的参数进行修改。其中数据库连接本身是指针类型，所以参数需要通过双指针才能对其进行修改。
connectionRAII::connectionRAII(MYSQL **con,connection_pool *connPool){
    *con=connPool->GetConnection();

    conRAII=*con;
    poolRAII=connPool;
}
connectionRAII::~connectionRAII(){
    poolRAII->ReleaseConnection(conRAII);
}

