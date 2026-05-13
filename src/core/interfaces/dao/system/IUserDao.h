#ifndef IUSERDAO_H
#define IUSERDAO_H

#include <QList>
#include "models/po/IUser.h"

/**
 * @brief 用户DAO接口
 */
class IUserDao
{
public:
    virtual ~IUserDao() = default;

    // 初始化表结构
    virtual bool initTable() = 0;

    // 查询所有用户
    virtual QList<IUser> findAll() = 0;

    // 根据ID查询用户
    virtual IUser findById(qint64 id) = 0;

    // 添加单个用户
    virtual bool addUser(const IUser& user) = 0;

    // 批量添加用户(事务安全)
    virtual bool batchAddUsers(const QList<IUser>& users) = 0;

    // 更新用户信息
    virtual bool updateUser(const IUser& user) = 0;

    // 根据ID删除用户
    virtual bool deleteById(qint64 id) = 0;
};

#endif // IUSERDAO_H
