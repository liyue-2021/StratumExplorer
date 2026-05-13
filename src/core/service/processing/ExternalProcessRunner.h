// =============================================================================
//  ExternalProcessRunner.h
//  作者：工程师 ly
//
//  跨进程算法调用器：把一个 IProcessNode 任务封装为一次外部 EXE 调用。
//  - 单任务单进程模型：父进程通过 stdin 投一行 JSON 描述任务参数 / 输入 /
//    输出路径，子进程通过 stdout 按行回 JSON 事件（progress / log / done /
//    error），stderr 视为自由日志（转发给 LogBus）
//  - 提供阻塞式 runBlocking()：内部跑 QEventLoop，避免节点实现感知线程
//  - 提供 requestCancel()：通过 std::atomic 标志 + 200ms 轮询触发
//    QProcess::terminate()，5s 内不退则 kill()
//  - 算法团队只需让自己的 EXE 实现这个 JSON 协议即可被任意节点复用
// =============================================================================
#ifndef PROCESSING_EXTERNAL_PROCESS_RUNNER_H
#define PROCESSING_EXTERNAL_PROCESS_RUNNER_H

#include "service/processing/IProcessNode.h"

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QHash>
#include <atomic>
#include <functional>

class QProcess;

namespace processing {

/**
 * @brief 跨进程算法调用的统一封装。
 *
 * ───────────────────────────────────────────────────────────────────
 * 协议（与算法 exe 约定，单任务一进程，进程结束即任务结束）
 *   stdin  : 一行 JSON
 *     {
 *       "op"     : "run",
 *       "params" : { ...节点参数... },
 *       "inputs" : { "in":  ["a.h5"], "in2": ["b.h5"] },
 *       "output" : "C:/tmp/oilpro/<nodeId>.h5"
 *     }
 *   stdout : 多行 JSON（每行一条事件）
 *     {"event":"progress","value":42,"message":"..."}
 *     {"event":"log","message":"..."}
 *     {"event":"done","outputs":["C:/tmp/oilpro/<nodeId>.h5"]}
 *     {"event":"error","message":"..."}
 *   stderr : 自由文本（直接进日志面板，便于算法侧 printf 调试）
 *   exit   : 0 = 成功；非 0 = 失败
 * ───────────────────────────────────────────────────────────────────
 *
 * 实现策略：
 *   - 同步 API：在工作线程里调用 runBlocking()，内部用本地事件循环把 QProcess
 *     转成"等待退出"的同步语义，对调用方而言是一个普通的阻塞函数。
 *   - 取消：requestCancel() → QProcess::terminate()（5s 内未退出再 kill()）。
 *   - 工程位置：暂时把 exe 路径硬编码为节点参数 "exePath"，后续接入算法组
 *     可改成统一的 ToolRegistry/全局设置。
 */
class ExternalProcessRunner {
public:
    using ProgressFn = IProcessNode::ProgressCallback;

    struct TaskRequest {
        QString                          exePath;     // 算法可执行文件绝对路径
        QStringList                      extraArgs;   // 命令行附加参数（可选）
        QVariantMap                      params;      // 节点参数
        QHash<QString, QStringList>      inputs;      // 端口 → 文件列表
        QString                          outputPath;  // 期望输出路径（算法可忽略并自报）
        QString                          workingDir;  // 工作目录（默认 exe 所在目录）
    };

    struct TaskResult {
        NodeStatus  status = NodeStatus::Failed;
        QString     message;
        QStringList outputs;
    };

    ExternalProcessRunner();
    ~ExternalProcessRunner();

    // 同步运行一个外部任务。在工作线程调用。
    TaskResult runBlocking(const TaskRequest& req, ProgressFn onProgress);

    // 线程安全的取消请求。runBlocking 进程会被 terminate → kill。
    void requestCancel();
    bool isCancelRequested() const { return m_cancel.load(); }

private:
    std::atomic<bool> m_cancel{false};
    QProcess*         m_proc = nullptr;   // 仅在 runBlocking 内部生命周期使用
};

} // namespace processing

#endif
