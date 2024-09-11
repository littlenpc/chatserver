#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// GroupUser表 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    // 群组中用户的角色：管理员/普通成员
    string role;
};

#endif