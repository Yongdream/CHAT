#include "groupmodel.hpp"
#include "db.hpp"
#include<vector>

using namespace std;

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str()); // 向群组表中插入群组名和群组描述信息
 
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection())); // 数据库自动生成id号，设置群组id
            return true;
        }
    }
    return false;
}

// 加入群组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values('%d', '%d', '%s')",
            groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1. 根据 userid 在 groupuser 表中查询出该用户所属的「群组消息」
    2. 根据「群组消息」，查询属于该群组的所有用户的 userid
    3. 利用群内userid 和 user 表进行多表联合查询，查出用户各个群「群内信息」
    */

    char sql[1024] = {0};
    
    // 「select ** from allgroup a」从 allgroup 表中选择数据，并命名为 a
    // 「inner join」执行内连接，将 allgroup 表中的 id 列与 groupuser 表中的 groupid 列匹配
    // 「where b.userid = %d」 选择 groupuser 表中 userid 行
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a\
            inner join groupuser b on a.id = b.groupid\
            where b.userid=%d", 
            userid);
    
    vector<Group> groupVec;  //  定义数组存放群组对象

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;  // 查出userid所有的群组信息
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &group : groupVec)
    {
        // 多表联合查询，获取不同群组中所有用户信息，并添加至对应群组的用户数组中
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
                inner join groupuser b on b.userid = a.id \
                where b.groupid=%d",
                group.getId());
        
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);   
        }
    }
    return groupVec;
}


// 查询群聊用户
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid); 
    // 获取群组中除自身userid之外的所有用户id
 
    vector<int> idVec; // 定义数组存放群组用户id
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}