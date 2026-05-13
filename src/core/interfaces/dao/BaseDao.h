#ifndef BASEDAO_H
#define BASEDAO_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMap>
#include <QList>
#include "dao/DatabaseManager.h"

/**
 * @brief 通用DAO基类
 * 🔥 无任何具体实体依赖，支持所有业务对象
 */
class BaseDAO
{
public:
    explicit BaseDAO(DatabaseManager* manager) : m_manager(manager) {}
    virtual ~BaseDAO() = default;

    // 禁用拷贝
    BaseDAO(const BaseDAO&) = delete;
    BaseDAO& operator=(const BaseDAO&) = delete;
    virtual bool initTable() = 0; // 纯虚 = 子类必须实现！

protected:
    // ===================== 线程安全获取连接 =====================
    QSqlDatabase db() {
        return m_manager->getDatabase();
    }

    // ===================== 基础SQL执行 =====================
    bool exec(const QString& sql) {
        return m_manager->exec(sql);
    }
    bool exec(const QString& sql, const QMap<QString, QVariant>& params) {
        return m_manager->exec(sql, params);
    }
    QSqlQuery query(const QString& sql) {
        return m_manager->query(sql);
    }
    QSqlQuery query(const QString& sql, const QMap<QString, QVariant>& params) {
        return m_manager->query(sql, params);
    }
    QString lastError() const {
        return m_manager->lastError();
    }

    // ===================== 事务 =====================
    bool beginTransaction() { return db().transaction(); }
    bool commit() { return db().commit(); }
    bool rollback() { return db().rollback(); }

    // ===================== 🔥 通用结果转换（无实体依赖） =====================
    // 单条 -> 对象
    template<typename T>
    T toObject(QSqlQuery& query, std::function<void(QSqlQuery&, T&)> mapper) {
        T obj;
        if (query.next()) {
            mapper(query, obj);
        }
        return obj;
    }

    // 多条 -> 对象列表
    template<typename T>
    QList<T> toList(QSqlQuery& query, std::function<void(QSqlQuery&, T&)> mapper) {
        QList<T> list;
        while (query.next()) {
            T obj;
            mapper(query, obj);
            list.append(obj);
        }
        return list;
    }

private:
    DatabaseManager* m_manager;
};

#endif // BASEDAO_H
