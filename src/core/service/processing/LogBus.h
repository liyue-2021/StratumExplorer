// =============================================================================
//  LogBus.h
//  作者：工程师 ly
//
//  进程内全局日志总线（QObject 单例）。
//  - 任意线程通过 LogBus::instance().post(level, source, message) 投递日志
//  - 内部加锁写入 %APPDATA%/StratumExplorer/logs/run_YYYYMMDD.log（按天滚动）
//  - 同时 emit logged(...) 信号，由 GUI 端的 LogDock 订阅显示
//  - 设计目的：让 WorkflowEngine / 节点 / 跨进程算法 等所有日志统一出口，
//    GUI 不耦合任何具体引擎实例
// =============================================================================
#ifndef PROCESSING_LOG_BUS_H
#define PROCESSING_LOG_BUS_H

#include <QObject>
#include <QString>
#include <QDateTime>

namespace processing {

enum class LogLevel { Info, Warn, Error, Debug };

/**
 * @brief 进程内日志总线（单例）。
 *
 * 全局唯一入口：所有节点 / 引擎 / 跨进程算法的日志都发到这里，
 * 由 GUI 层的 LogDock 监听并显示，同时落盘到 %APPDATA%/StratumExplorer/logs/。
 *
 * 特意做成 QObject 单例 + 信号广播，避免 GUI 强依赖某个具体 Engine 实例
 * （多 Tab 多 engine 时仍然只有一个日志窗口）。
 *
 * 线程安全：emit 信号是 Qt::AutoConnection，跨线程会自动排队到接收者线程。
 */
class LogBus : public QObject {
    Q_OBJECT
public:
    static LogBus& instance();

    // 任意线程可调用
    void post(LogLevel level,
              const QString& source,    // 来源标签：node id、"engine"、"runner" 等
              const QString& message);

signals:
    void logged(const QDateTime& time,
                processing::LogLevel level,
                const QString& source,
                const QString& message);

private:
    LogBus();
    Q_DISABLE_COPY(LogBus)
};

} // namespace processing

#endif
