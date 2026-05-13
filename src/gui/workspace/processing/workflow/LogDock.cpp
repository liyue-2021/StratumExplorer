#include "LogDock.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBlock>
#include <QVBoxLayout>

using namespace processing::gui;
using processing::LogLevel;

namespace {
const char* levelTag(LogLevel l) {
    switch (l) {
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERR";
        case LogLevel::Debug: return "DBG";
    }
    return "INFO";
}
QString levelColor(LogLevel l) {
    switch (l) {
        case LogLevel::Info:  return "#37474F";
        case LogLevel::Warn:  return "#E65100";
        case LogLevel::Error: return "#C62828";
        case LogLevel::Debug: return "#607D8B";
    }
    return "#37474F";
}
} // namespace

LogDock::LogDock(QWidget* parent) : QWidget(parent) {
    m_level = new QComboBox(this);
    m_level->addItems({tr("全部"), tr("INFO"), tr("WARN"), tr("ERROR"), tr("DEBUG")});
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("过滤来源 / 关键字…"));
    m_clear = new QPushButton(tr("清空"), this);

    auto* top = new QHBoxLayout;
    top->addWidget(m_level);
    top->addWidget(m_filter, 1);
    top->addWidget(m_clear);

    m_view = new QPlainTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setMaximumBlockCount(m_maxLines);
    QFont mono("Consolas"); mono.setStyleHint(QFont::Monospace);
    m_view->setFont(mono);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->addLayout(top);
    root->addWidget(m_view, 1);

    connect(m_clear, &QPushButton::clicked, m_view, &QPlainTextEdit::clear);
    connect(&processing::LogBus::instance(), &processing::LogBus::logged,
            this, &LogDock::onLogged);
}

void LogDock::onLogged(const QDateTime& time, LogLevel level,
                       const QString& source, const QString& message) {
    // 等级过滤
    const int sel = m_level->currentIndex(); // 0=全部
    if (sel > 0) {
        const LogLevel want = static_cast<LogLevel>(sel - 1);
        if (level != want) return;
    }
    // 关键字过滤
    const QString f = m_filter->text().trimmed();
    if (!f.isEmpty()
        && !source.contains(f, Qt::CaseInsensitive)
        && !message.contains(f, Qt::CaseInsensitive)) {
        return;
    }

    const QString html = QStringLiteral(
        "<span style='color:#90A4AE'>%1</span> "
        "<span style='color:%2;font-weight:bold'>[%3]</span> "
        "<span style='color:#1976D2'>[%4]</span> %5")
        .arg(time.toString("HH:mm:ss.zzz"),
             levelColor(level),
             QString::fromLatin1(levelTag(level)),
             source.toHtmlEscaped(),
             message.toHtmlEscaped());
    appendLine(html);
}

void LogDock::appendLine(const QString& html) {
    m_view->appendHtml(html);
    auto bar = m_view->verticalScrollBar();
    if (bar) bar->setValue(bar->maximum());
}
