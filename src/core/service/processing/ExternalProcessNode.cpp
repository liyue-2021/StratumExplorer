// =============================================================================
//  ExternalProcessNode.cpp
//  作者：工程师 ly
//
//  通用外部进程节点的实现。把 IProcessNode::run() 转为
//  ExternalProcessRunner::runBlocking()，屏蔽所有跨进程细节。
//
//  协议：前端生成 input_config_<id>.json，EXE 唯一参数为该路径；
//        业务 HDF5 由后端读写。展示类节点（externalProcess=false）不启动 EXE。
// =============================================================================
#include "ExternalProcessNode.h"

#include <QDir>
#include <QStandardPaths>

namespace processing
{

    ExternalProcessNode::ExternalProcessNode(NodeMeta meta)
        : m_meta(std::move(meta))
    {
        // 仅外部算法节点需要 exePath 占位，供属性面板填写
        if (m_meta.externalProcess
            && !m_meta.defaultParams.contains(QStringLiteral("exePath")))
        {
            m_meta.defaultParams.insert(QStringLiteral("exePath"), QString());
        }
        m_params = m_meta.defaultParams;
    }

    const NodeMeta &ExternalProcessNode::meta() const { return m_meta; }

    QString ExternalProcessNode::instanceId() const { return m_id; }
    void ExternalProcessNode::setInstanceId(const QString &id) { m_id = id; }

    QVariantMap ExternalProcessNode::params() const { return m_params; }
    void ExternalProcessNode::setParams(const QVariantMap &p) { m_params = p; }

    void ExternalProcessNode::setInputFiles(const QString &portKey,
                                            const QStringList &files)
    {
        m_inputs[portKey] = files;
    }

    NodeRunResult ExternalProcessNode::run(ProgressCallback onProgress)
    {
        // 展示类节点：不调用外部 EXE，透传上游输入供 PlotDialog 等 UI 使用
        if (!m_meta.externalProcess)
        {
            QStringList outs;
            for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it)
                outs.append(it.value());
            if (onProgress)
                onProgress(100, QStringLiteral("展示节点（无需运行算法）"));
            return {NodeStatus::Succeeded,
                    QStringLiteral("展示节点已就绪，请在结果视图中查看"),
                    outs};
        }

        // ---- 1. 拿 EXE 路径 ----
        const QString exePath = m_params.value(QStringLiteral("exePath")).toString();
        if (exePath.isEmpty())
        {
            return {NodeStatus::Failed,
                    QStringLiteral("节点 [%1] 未配置 exePath 参数").arg(m_meta.displayName),
                    {}};
        }

        // ---- 2. 组装 TaskRequest（HDF5 协议）----
        ExternalProcessRunner::TaskRequest req;
        req.exePath = exePath;
        req.taskId = m_id.isEmpty() ? 0 : qHash(m_id); // 生成唯一任务 ID
        req.funcId = m_meta.funcId;                    // 功能编号 0~41
        req.funcName = m_meta.funcName;                // Python 函数名

        // 从 m_inputs 收集所有输入文件路径（合并所有端口）
        QStringList allInputs;
        for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it)
        {
            allInputs.append(it.value());
        }
        req.inputPaths = allInputs;

        // 从 params 中提取算法参数（移除前端管理字段）
        req.params = m_params;
        req.params.remove(QStringLiteral("exePath"));

        req.outputPath = buildOutputPath();

        // 可选路径参数（来自 params）
        if (req.params.contains(QStringLiteral("ref_depth_path")))
        {
            req.refDepthPath = req.params.take(QStringLiteral("ref_depth_path")).toString();
        }
        if (req.params.contains(QStringLiteral("calib_path")))
        {
            req.calibPath = req.params.take(QStringLiteral("calib_path")).toString();
        }
        if (req.params.contains(QStringLiteral("temp_path")))
        {
            req.tempPath = req.params.take(QStringLiteral("temp_path")).toString();
        }
        if (req.params.contains(QStringLiteral("ref_path")))
        {
            req.refPath = req.params.take(QStringLiteral("ref_path")).toString();
        }

        // ---- 3. 委托给 Runner ----
        ExternalProcessRunner::TaskResult tr = m_runner.runBlocking(req, onProgress);

        // ---- 4. 转换结果 ----
        QStringList outputs;
        if (!tr.dataFilePath.isEmpty())
        {
            outputs.append(tr.dataFilePath);
        }
        outputs.append(tr.outputPaths);

        return {tr.status, tr.message, outputs};
    }

    void ExternalProcessNode::requestCancel() { m_runner.requestCancel(); }
    bool ExternalProcessNode::isCancelRequested() const { return m_runner.isCancelRequested(); }

    QString ExternalProcessNode::buildOutputPath() const
    {
        // %TEMP%/oilpro/
        const QString tmpDir = QDir::tempPath() + QStringLiteral("/oilpro");
        QDir().mkpath(tmpDir);
        return tmpDir;
    }

} // namespace processing
