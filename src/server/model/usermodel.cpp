#include "usermodel.hpp"
#include "db.hpp"
#include <iostream>

using namespace std;
 
// User表的增加方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
 
    MySQL mysql;         // 定义一个mysql对象
    if (mysql.connect()) // 连接成功了
    {
        if (mysql.update(sql)) // 更新这个sql语句传进去
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection())); 
            // 拿到数据库生成的id作为用户的id号
            return true;
        }
    }
 
    return false;
}