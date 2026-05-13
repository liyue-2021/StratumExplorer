#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QThread>

/**
 * @brief 非单例、线程安全的 SQLite 数据库管理器
 * 功能：默认数据源、线程内切换数据库、线程安全操作
 */
class DatabaseManager
{
public:
    /**
     * @brief 构造函数
     * @param defaultDbPath 默认数据库路径（未手动切换时使用）
     *                     内存数据库：":memory:"（推荐测试使用）
     *                     文件数据库："./default.db"
     */
    explicit DatabaseManager(const QString& defaultDbPath = "./default.db");
    ~DatabaseManager();

    // 禁用拷贝构造和赋值（避免数据库连接重复）
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // ------------------- 核心接口 -------------------
    /**
     * @brief 线程安全切换数据库（仅作用于当前调用线程）
     * @param dbPath 新数据库路径
     * @return 切换成功返回 true，失败返回 false
     */
    bool switchDatabase(const QString& dbPath);

    /**
     * @brief 获取当前线程的数据库连接（自动使用默认数据源）
     * @return 可用的 QSqlDatabase 连接
     */
    QSqlDatabase getDatabase();

    /**
     * @brief 关闭当前线程的数据库连接
     */
    void closeCurrentConnection();

    // ------------------- 工具方法（CRUD） -------------------
    // 执行SQL语句（无结果）
    bool exec(const QString& sql);
    // 执行带参数的SQL语句（防注入）
    bool exec(const QString& sql, const QMap<QString, QVariant>& params);
    // 查询数据
    QSqlQuery query(const QString& sql);
    QSqlQuery query(const QString& sql, const QMap<QString, QVariant>& params);
    // 获取最后一次错误信息
    QString lastError() const;

private:
    /**
     * @brief 生成当前线程唯一的数据库连接名（线程安全核心）
     * @return 连接名：格式 "sqlite_thread_线程ID"
     */
    QString getConnectionName() const;

    /**
     * @brief 打开数据库连接
     * @param connectionName 唯一连接名
     * @param dbPath 数据库路径
     * @return 打开成功返回 true
     */
    bool openDatabase(const QString& connectionName, const QString& dbPath);

private:
    QString m_defaultDbPath;  // 默认数据源路径
    mutable QString m_lastError; // 最后一次错误信息
};

#endif // DATABASEMANAGER_H
