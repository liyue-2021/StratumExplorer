#include "LogBus.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>

namespace processing {

namespace {
QMutex g_fileMutex;
QFile* g_file = nullptr;

QFile* dailyFile() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + "/logs");
    const QString path = QStringLiteral("%1/logs/run_%2.log")
        .arg(base, QDateTime::currentDateTime().toString("yyyyMMdd"));
    if (g_file && g_file->fileName() == path && g_file->isOpen()) return g_file;
    if (g_file) { g_file->close(); delete g_file; g_file = nullptr; }
    g_file = new QFile(path);
    if (!g_file->open(QIODevice::Append | QIODevice::Text)) {
        delete g_file; g_file = nullptr;
    }
    return g_file;
}

const char* levelStr(LogLevel l) {
    switch (l) {
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERR ";
        case LogLevel::Debug: return "DBG ";
    }
    return "INFO";
}
} // namespace

LogBus& LogBus::instance() {
    static LogBus inst;
    return inst;
}

LogBus::LogBus() = default;

void LogBus::post(LogLevel level, const QString& source, const QString& message) {
    const QDateTime now = QDateTime::currentDateTime();

    // 落盘
    {
        QMutexLocker lock(&g_fileMutex);
        if (auto* f = dailyFile()) {
            QTextStream ts(f);
            ts << now.toString("HH:mm:ss.zzz") << " ["
               << levelStr(level) << "] [" << source << "] " << message << "\n";
            ts.flush();
        }
    }

    emit logged(now, level, source, message);
}

} // namespace processing
