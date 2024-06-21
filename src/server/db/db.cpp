#include "db.hpp"
#include <muduo/base/Logging.h>
 
//数据库配置信息
static string server = "192.168.122.129";
static string user = "root1";
static string password = "qwer1234";
static string dbname = "chat";
 
//初始化数据库连接
MySQL::MySQL()
{
    conn_ = mysql_init(nullptr);
}
 
//释放数据库连接资源
MySQL::~MySQL()
{
    if (conn_ != nullptr)
        mysql_close(conn_);
}
 
// 连接数据库
bool MySQL::connect()
{
    // mysql_options(conn_, MYSQL_OPT_RECONNECT, 0);

    LOG_INFO << "Attempting to connect to MySQL server...";
    MYSQL *p = mysql_real_connect(conn_, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        //C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(conn_, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
        LOG_INFO << "connect mysql fail! Error: " << mysql_error(conn_);
    }
 
    return p;
}
 
//更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(conn_, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << " 更新失败! Error: " << mysql_error(conn_);
        return false;
    }
 
    return true;
}
 
// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(conn_, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    
    return mysql_use_result(conn_);
}
 
//获取连接
MYSQL* MySQL::getConnection()
{
    return conn_;
}