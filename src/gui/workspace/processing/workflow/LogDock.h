// =============================================================================
//  LogDock.h
//  作者：工程师 ly
//
//  工作流编辑器底部的日志面板（QWidget，由 WorkflowEditorTab 嵌入到
//  垂直 QSplitter 下半区）。
//  - 监听全局 processing::LogBus::logged 信号
//  - 支持等级下拉过滤（全部 / INFO / WARN / ERROR / DEBUG）
//  - 支持来源/关键字模糊过滤
//  - 行数封顶 5000，溢出自动滚动丢弃，避免长跑导致内存膨胀
//  - 提供「清空」按钮，落盘日志由 LogBus 自行管理
// =============================================================================
#ifndef PROCESSING_GUI_LOG_DOCK_H
#define PROCESSING_GUI_LOG_DOCK_H

#include <QWidget>
#include <QDateTime>

#include "LogBus.h"

class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QLineEdit;

namespace processing { namespace gui {

/**
 * @brief 全局日志面板（监听 LogBus 单例）。
 *
 * 设计要点：
 *   - 不依赖任何 Engine 实例，挂在主窗口下方的 QDockWidget 即可
 *   - 自带等级过滤、来源过滤、清空、跳到最新
 *   - 行数封顶 5000，溢出从头丢弃，避免长跑导致内存膨胀
 */
class LogDock : public QWidget {
    Q_OBJECT
public:
    explicit LogDock(QWidget* parent = nullptr);

private slots:
    void onLogged(const QDateTime& time,
                  processing::LogLevel level,
                  const QString& source,
                  const QString& message);

private:
    void appendLine(const QString& html);

    QPlainTextEdit* m_view  = nullptr;
    QComboBox*      m_level = nullptr;
    QLineEdit*      m_filter = nullptr;
    QPushButton*    m_clear = nullptr;
    int             m_maxLines = 5000;
};

}} // namespace processing::gui

#endif
