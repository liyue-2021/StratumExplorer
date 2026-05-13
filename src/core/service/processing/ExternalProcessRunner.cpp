#include "ExternalProcessRunner.h"

#include <QProcess>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>

namespace processing {

ExternalProcessRunner::ExternalProcessRunner()  = default;
ExternalProcessRunner::~ExternalProcessRunner() = default;

void ExternalProcessRunner::requestCancel() {
    m_cancel.store(true);
    // m_proc 由 runBlocking 在自己线程里管理；这里仅设置标志，
    // 由 runBlocking 内部的轮询/事件循环检查后调用 terminate/kill。
}

ExternalProcessRunner::TaskResult
ExternalProcessRunner::runBlocking(const TaskRequest& req, ProgressFn onProgress) {
    TaskResult r;
    m_cancel.store(false);

    if (req.exePath.isEmpty() || !QFileInfo::exists(req.exePath)) {
        r.status  = NodeStatus::Failed;
        r.message = QStringLiteral("算法可执行文件不存在: %1").arg(req.exePath);
        return r;
    }

    QProcess proc;
    m_proc = &proc;
    proc.setProgram(req.exePath);
    proc.setArguments(req.extraArgs);
    proc.setWorkingDirectory(req.workingDir.isEmpty()
                             ? QFileInfo(req.exePath).absolutePath()
                             : req.workingDir);

    // ---- 组装 stdin 任务包 ----
    QJsonObject taskObj;
    taskObj["op"] = "run";
    taskObj["params"] = QJsonObject::fromVariantMap(req.params);
    QJsonObject inObj;
    for (auto it = req.inputs.constBegin(); it != req.inputs.constEnd(); ++it) {
        QJsonArray arr; for (const auto& f : it.value()) arr.append(f);
        inObj[it.key()] = arr;
    }
    taskObj["inputs"] = inObj;
    taskObj["output"] = req.outputPath;
    const QByteArray taskJson = QJsonDocument(taskObj).toJson(QJsonDocument::Compact) + "\n";

    // ---- 用本地事件循环把异步 QProcess 转成同步 ----
    QEventLoop loop;
    bool finished = false;
    int  exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;

    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]{
        // 算法侧每行一条 JSON 事件
        while (proc.canReadLine()) {
            const QByteArray line = proc.readLine().trimmed();
            if (line.isEmpty()) continue;
            const auto doc = QJsonDocument::fromJson(line);
            if (!doc.isObject()) {
                // 不是合法 JSON 当作日志透传
                if (onProgress) onProgress(-1, QString::fromUtf8(line));
                continue;
            }
            const auto o = doc.object();
            const auto ev = o.value("event").toString();
            if (ev == "progress") {
                if (onProgress) {
                    onProgress(o.value("value").toInt(),
                               o.value("message").toString());
                }
            } else if (ev == "log") {
                if (onProgress) onProgress(-1, o.value("message").toString());
            } else if (ev == "done") {
                for (const auto& v : o.value("outputs").toArray())
                    r.outputs << v.toString();
                r.status = NodeStatus::Succeeded;
                r.message = QStringLiteral("ok");
            } else if (ev == "error") {
                r.status = NodeStatus::Failed;
                r.message = o.value("message").toString();
            }
        }
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, [&]{
        const QByteArray err = proc.readAllStandardError();
        if (!err.isEmpty() && onProgress) {
            onProgress(-1, QStringLiteral("[stderr] %1").arg(QString::fromUtf8(err).trimmed()));
        }
    });
    QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [&](int code, QProcess::ExitStatus st){
        exitCode = code; exitStatus = st; finished = true;
        loop.quit();
    });

    // 周期性检查取消标志：terminate → 5s 后 kill
    QTimer cancelTick;
    cancelTick.setInterval(200);
    bool terminateSent = false;
    qint64 termAtMs = 0;
    QObject::connect(&cancelTick, &QTimer::timeout, [&]{
        if (!m_cancel.load()) return;
        if (!terminateSent && proc.state() == QProcess::Running) {
            proc.terminate();
            terminateSent = true;
            termAtMs = QDateTime::currentMSecsSinceEpoch();
        } else if (terminateSent && proc.state() == QProcess::Running) {
            if (QDateTime::currentMSecsSinceEpoch() - termAtMs > 5000) {
                proc.kill();
            }
        }
    });
    cancelTick.start();

    proc.start();
    if (!proc.waitForStarted(5000)) {
        m_proc = nullptr;
        r.status  = NodeStatus::Failed;
        r.message = QStringLiteral("无法启动: %1").arg(proc.errorString());
        return r;
    }
    proc.write(taskJson);
    proc.closeWriteChannel();   // 通知算法 stdin 结束

    if (!finished) loop.exec();

    cancelTick.stop();
    m_proc = nullptr;

    // 兜底：算法没主动报 done 但退出码 0，也认为成功
    if (exitStatus == QProcess::NormalExit && exitCode == 0
        && r.status != NodeStatus::Succeeded && r.status != NodeStatus::Stopped) {
        r.status  = NodeStatus::Succeeded;
        if (r.outputs.isEmpty() && !req.outputPath.isEmpty()) r.outputs << req.outputPath;
        if (r.message.isEmpty()) r.message = QStringLiteral("ok (assumed)");
    }
    if (exitStatus == QProcess::CrashExit) {
        r.status  = NodeStatus::Failed;
        r.message = QStringLiteral("算法进程崩溃 (exit=%1)").arg(exitCode);
    }
    if (m_cancel.load()) {
        r.status  = NodeStatus::Stopped;
        r.message = QStringLiteral("用户停止");
    }
    return r;
}

} // namespace processing
