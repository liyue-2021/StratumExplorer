#ifndef IUSER_H
#define IUSER_H

#include <QString>
#include <QDateTime>

/**
 * @brief 用户实体类
 * 对应数据库表：sys_user
 */
class IUser
{
public:
    IUser() : m_id(0), m_status(1) {}

    // Getter & Setter
    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString username() const { return m_username; }
    void setUsername(const QString& username) { m_username = username; }

    QString password() const { return m_password; }
    void setPassword(const QString& password) { m_password = password; }

    QString nickname() const { return m_nickname; }
    void setNickname(const QString& nickname) { m_nickname = nickname; }

    QString phone() const { return m_phone; }
    void setPhone(const QString& phone) { m_phone = phone; }

    QDateTime createTime() const { return m_createTime; }
    void setCreateTime(const QDateTime& createTime) { m_createTime = createTime; }

    int status() const { return m_status; }
    void setStatus(int status) { m_status = status; }

private:
    qint64      m_id;          // 主键ID
    QString     m_username;    // 用户名(唯一)
    QString     m_password;    // 密码
    QString     m_nickname;    // 昵称
    QString     m_phone;       // 手机号
    QDateTime   m_createTime;  // 创建时间
    int         m_status;      // 状态 1-正常 0-禁用
};

#endif // IUSER_H
