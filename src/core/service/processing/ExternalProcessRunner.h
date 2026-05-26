// =============================================================================
//  ExternalProcessRunner.h
//  作者：工程师 ly
//
//  跨进程算法调用器：把一个 IProcessNode 任务封装为一次外部 EXE 调用。
//  - 单任务单进程：前端生成任务配置文件（默认 JSON），作为 EXE 唯一命令行参数；
//    EXE 完成后在 output 目录写入任务结果文件（默认同为 JSON，见 OILPRO_PROTOCOL）。
//  - 前端不负责 HDF5 二进制读写；若后端要求真 .h5，由后端在 EXE 内解析/生成（或开启 USE_HDF5）。
//  - 提供阻塞式 runBlocking()：内部跑 QEventLoop，避免节点实现感知线程
//  - 提供 requestCancel()：通过 std::atomic 标志 + 200ms 轮询触发
//    QProcess::terminate()，5s 内不退则 kill()
//  - 算法团队只需让自己的 EXE 实现 HDF5 协议即可被任意节点复用
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

namespace processing
{

    /**
     * @brief 跨进程算法调用的统一封装。
     *
     * ───────────────────────────────────────────────────────────────────
     * 协议（与算法 EXE 约定，单任务一进程）
     *   输入：主程序启动 EXE，传入 input_config_<taskId>.json 路径（唯一参数；USE_HDF5 时为 .h5）
     *   输出：EXE 在 outputPath 下写入 task_<funcId>_<taskId>.h5（JSON 协议时为 .json）
     *
     * 前端职责：组装下列字段并写入配置文件；不解析业务 HDF5 数据集。
     * 后端职责：读配置、跑算法、写结果文件（及业务 HDF5 数据文件）。
     *
     * input_config 逻辑结构（当前默认 JSON 文件；字段名与 HDF5 组路径一致）：
     *   /task_info/task_id        int32   任务标识
     *   /task_info/func_id       int32   功能编号 0~41
     *   /task_info/func_name     string  功能名称（如 func_depth_correct）
     *   /io/input_path           string[] 输入数据文件路径（业务 HDF5，由后端读写）
     *   /io/output_path          string   输出目录
     *   /io/ref_depth_path       string   参考深度（深度矫正）
     *   /io/calib_path           string   标定文件（DTS 温度矫正）
     *   /io/temp_path            string   同步温度（DSS 温耦分离）
     *   /io/ref_path             string   参考基线（DSS 基线校准）
     *   （上列 6 个 IO 路径由 TaskRequest 对应字段填入，见 ExternalProcessNode::run）
     *   /params/                 {...}    算法运行参数（各模块自定义）
     *
     * task_<funcId>_<taskId> 结果文件结构：
     *   /task_info/task_id       int      回传任务标识
     *   /task_info/func_id       int      功能编号
     *   /task_info/func_name     string   功能名称
     *   /io/input_path           string[] 输入数据文件路径
     *   /io/output_path          string[] 输出数据文件路径
     *   /io/data_file_path       string   实际输出的数据文件路径
     *   /status/code             int      0=成功, 1=失败
     *   /status/message          string   状态描述/错误信息
     *   /status/error            string   错误信息（失败时）
     *   /time/start_time         string   任务开始时间
     *   /time/end_time           string   任务结束时间
     *   /time/duration           float    任务运行时间（秒）
     *   /params/                 {...}    算法参数回显
     * ───────────────────────────────────────────────────────────────────
     *
     * 实现策略：
     *   - 同步 API：在工作线程里调用 runBlocking()，内部用本地事件循环把 QProcess
     *     转成"等待退出"的同步语义，对调用方而言是一个普通的阻塞函数。
     *   - 取消：requestCancel() → QProcess::terminate()（5s 内未退出再 kill()）。
     *   - 默认构建写 JSON 配置；USE_HDF5 时由 H5Cpp 写二进制（可选，后端亦可只认 JSON）。
     */
    class ExternalProcessRunner
    {
    public:
        using ProgressFn = IProcessNode::ProgressCallback;

        struct TaskRequest
        {
            QString exePath;        // 算法可执行文件绝对路径
            QStringList extraArgs;  // 命令行附加参数（可选）
            int taskId;             // 任务唯一标识
            int funcId;             // 功能编号 0~41（ProductionNodes）
            QString funcName;       // 功能名称（如 func_depth_correct）
            QStringList inputPaths; // 输入数据文件路径列表
            QString outputPath;     // 输出文件根路径
            QString refDepthPath;   // 参考深度文件路径（可选）
            QString calibPath;      // 标定文件路径（可选）
            QString tempPath;       // 同步温度数据路径（可选）
            QString refPath;        // 参考基线路径（可选）
            QVariantMap params;     // 算法运行参数
            QString workingDir;     // 工作目录（默认 exe 所在目录）
        };

        struct TaskResult
        {
            NodeStatus status = NodeStatus::Failed;
            QString message;
            QString dataFilePath;    // 算法输出的数据文件路径
            QStringList outputPaths; // 所有输出文件路径
            int duration = 0;        // 运行时间（毫秒）
        };

        ExternalProcessRunner();
        ~ExternalProcessRunner();

        // 同步运行一个外部任务。在工作线程调用。
        TaskResult runBlocking(const TaskRequest &req, ProgressFn onProgress);

        // 线程安全的取消请求。runBlocking 进程会被 terminate → kill。
        void requestCancel();
        bool isCancelRequested() const { return m_cancel.load(); }

    private:
        // 生成 input_config 任务配置文件（默认 .json）
        bool writeInputConfig(const TaskRequest &req, const QString &configPath) const;

        // 读取 task_<funcId>_<taskId> 结果文件
        TaskResult readTaskResult(const QString &taskPath) const;

        std::atomic<bool> m_cancel{false};
        QProcess *m_proc = nullptr; // 仅在 runBlocking 内部生命周期使用
    };

} // namespace processing

#endif
