#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1 组装sql语句
    // sql里面的其实就是要执行的sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d', '%d')", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        // 将sql更新到mysql中
        mysql.update(sql);
    }
}

// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    // 1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userid);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        // 如果res不为空，代表查成功
        if (res != nullptr)
        {
            // 把userid用户的所有好友消息放入vec中返回
            MYSQL_ROW row;
            // 进行查询，找到则放入vec
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.emplace_back(user);
            }

            mysql_free_result(res);
            return vec;
        }
    }
    // 如果连接不成功，直接返回一个空的vec
    return vec;
}