#include "UserServiceImpl.h"
#include <QDebug>

UserServiceImpl::UserServiceImpl(IUserDao* userDao)
    : m_userDao(userDao)
{
}

bool UserServiceImpl::initUserTable()
{
    return m_userDao->initTable();
}

QList<IUser> UserServiceImpl::findAllUsers()
{
    return m_userDao->findAll();
}

IUser UserServiceImpl::findUserById(qint64 id)
{
    return m_userDao->findById(id);
}

// 带业务校验的添加
bool UserServiceImpl::addUser(const IUser& user)
{
    if (!validateUser(user)) {
        qWarning() << "用户数据校验失败！";
        return false;
    }
    return m_userDao->addUser(user);
}

bool UserServiceImpl::batchAddUsers(const QList<IUser>& users)
{
    return m_userDao->batchAddUsers(users);
}

bool UserServiceImpl::updateUser(const IUser& user)
{
    if (!validateUser(user) || user.id() <= 0) {
        qWarning() << "更新用户数据校验失败！";
        return false;
    }
    return m_userDao->updateUser(user);
}

bool UserServiceImpl::deleteUserById(qint64 id)
{
    if (id <= 0) return false;
    return m_userDao->deleteById(id);
}

// 私有：用户数据合法性校验
bool UserServiceImpl::validateUser(const IUser& user)
{
    return !user.username().isEmpty() && !user.password().isEmpty();
}
