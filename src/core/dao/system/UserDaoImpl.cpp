#include "UserDaoImpl.h"
#include <QVariant>

UserDaoImpl::UserDaoImpl(DatabaseManager* manager)
    : BaseDAO(manager)
{
}

// ===================== 表初始化 =====================
bool UserDaoImpl::initTable()
{
    const QString sql = R"(
        CREATE TABLE IF NOT EXISTS sys_user (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT NOT NULL UNIQUE,
            password    TEXT NOT NULL,
            nickname    TEXT,
            phone       TEXT,
            create_time TEXT,
            status      INTEGER DEFAULT 1
        );
    )";
    return exec(sql);
}

// ===================== 结果映射器 =====================
void UserDaoImpl::userMapper(QSqlQuery& query, IUser& user)
{
    user.setId(query.value("id").toLongLong());
    user.setUsername(query.value("username").toString());
    user.setPassword(query.value("password").toString());
    user.setNickname(query.value("nickname").toString());
    user.setPhone(query.value("phone").toString());
    user.setCreateTime(QDateTime::fromString(query.value("create_time").toString(), Qt::ISODate));
    user.setStatus(query.value("status").toInt());
}

// ===================== 查询全部 =====================
QList<IUser> UserDaoImpl::findAll()
{
    const QString sql = "SELECT * FROM sys_user ORDER BY id DESC";
    QSqlQuery query = this->query(sql);
    return toList<IUser>(query, [this](QSqlQuery& q, IUser& u) { userMapper(q, u); });
}

// ===================== 根据ID查询 =====================
IUser UserDaoImpl::findById(qint64 id)
{
    const QString sql = "SELECT * FROM sys_user WHERE id = :id";
    QMap<QString, QVariant> params;
    params[":id"] = id;
    QSqlQuery query = this->query(sql, params);
    return toObject<IUser>(query, [this](QSqlQuery& q, IUser& u) { userMapper(q, u); });
}

// ===================== 单条添加 =====================
bool UserDaoImpl::addUser(const IUser& user)
{
    const QString sql = R"(
        INSERT INTO sys_user
        (username, password, nickname, phone, create_time, status)
        VALUES
        (:username, :password, :nickname, :phone, :create_time, :status)
    )";

    QMap<QString, QVariant> params;
    params[":username"]    = user.username();
    params[":password"]    = user.password();
    params[":nickname"]    = user.nickname();
    params[":phone"]       = user.phone();
    params[":create_time"] = user.createTime().toString(Qt::ISODate);
    params[":status"]      = user.status();

    return exec(sql, params);
}

// ===================== 批量添加(事务安全) =====================
bool UserDaoImpl::batchAddUsers(const QList<IUser>& users)
{
    if (users.isEmpty()) return true;

    // 开启事务
    if (!beginTransaction()) {
        return false;
    }

    try {
        for (const IUser& user : users) {
            if (!addUser(user)) {
                rollback();
                return false;
            }
        }
        // 提交事务
        return commit();
    }
    catch (...) {
        rollback();
        return false;
    }
}

// ===================== 更新用户 =====================
bool UserDaoImpl::updateUser(const IUser& user)
{
    const QString sql = R"(
        UPDATE sys_user SET
            username    = :username,
            password    = :password,
            nickname    = :nickname,
            phone       = :phone,
            status      = :status
        WHERE id = :id
    )";

    QMap<QString, QVariant> params;
    params[":id"]          = user.id();
    params[":username"]    = user.username();
    params[":password"]    = user.password();
    params[":nickname"]    = user.nickname();
    params[":phone"]       = user.phone();
    params[":status"]      = user.status();

    return exec(sql, params);
}

// ===================== 根据ID删除 =====================
bool UserDaoImpl::deleteById(qint64 id)
{
    const QString sql = "DELETE FROM sys_user WHERE id = :id";
    QMap<QString, QVariant> params;
    params[":id"] = id;
    return exec(sql, params);
}
