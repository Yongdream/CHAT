#include "friendmodel.hpp"
#include "db.hpp"
 
// 添加好友关系
bool FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);    // 向friend表插入好友关系
 
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}
 
// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
 
    sprintf(sql, "select a.id,a.name,a.state from user a \
    inner join friend b on b.friendid = a.id where b.userid=%d", userid);   
    // 多表联合查询，返回「userid」的好友列表信息
 
    vector<User> Friendvec;             // 定义vector数组，存放userid的好友对象
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                Friendvec.push_back(user);
            }
            mysql_free_result(res);
            return Friendvec;
        }
    }
    return Friendvec;
}