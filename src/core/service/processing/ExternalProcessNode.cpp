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
#include <QFile>
#include <QFileInfo>
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
        // 非外部进程节点：数据输入 / 展示类
        if (!m_meta.externalProcess)
        {
            if (m_meta.kind == NodeKind::PureOutput)
            {
                QStringList outs;
                const QVariant filesVar = m_params.value(QStringLiteral("input_files"));
                if (filesVar.typeId() == QMetaType::QVariantList)
                {
                    for (const QVariant &item : filesVar.toList())
                    {
                        const QString path = item.toString().trimmed();
                        if (!path.isEmpty())
                            outs.append(path);
                    }
                }
                if (outs.isEmpty())
                {
                    return {NodeStatus::Failed,
                            QObject::tr("请先在属性面板选择输入文件"),
                            {}};
                }
                if (onProgress)
                    onProgress(100, QObject::tr("已加载 %1 个文件").arg(outs.size()));
                return {NodeStatus::Succeeded,
                        QObject::tr("数据输入就绪（%1 个文件）").arg(outs.size()),
                        outs};
            }

            // [后端] preprocess.format_convert：下列为 UI 联调占位，非真实转换。
            // 对接说明见 docs/BACKEND_HANDOFF.md §8.1（推荐改 externalProcess=true 走甲方 EXE）。
            if (m_meta.typeId == QLatin1String("preprocess.format_convert"))
            {
                QStringList inputs;
                for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it)
                    inputs.append(it.value());

                if (inputs.isEmpty())
                {
                    return {NodeStatus::Failed,
                            QObject::tr("请先将上游节点（如「数据输入」）连线并提供输入文件"),
                            {}};
                }

                const QString outDir =
                    m_params.value(QStringLiteral("output_dir")).toString().trimmed();
                if (outDir.isEmpty())
                {
                    return {NodeStatus::Failed,
                            QObject::tr("请在属性面板设置输出目录"),
                            {}};
                }

                QDir dir(outDir);
                if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
                {
                    return {NodeStatus::Failed,
                            QObject::tr("无法创建输出目录：%1").arg(outDir),
                            {}};
                }

                QStringList outs;
                int step = 0;
                const int total = inputs.size();
                for (const QString &inPath : inputs)
                {
                    ++step;
                    if (onProgress)
                    {
                        onProgress(step * 100 / total,
                                   QObject::tr("正在转换 %1/%2：%3")
                                       .arg(step)
                                       .arg(total)
                                       .arg(QFileInfo(inPath).fileName()));
                    }

                    const QFileInfo inFi(inPath);
                    const QString outPath =
                        dir.filePath(inFi.completeBaseName() + QStringLiteral(".h5"));

                    QFile inFile(inPath);
                    QFile outFile(outPath);
                    if (!inFile.open(QIODevice::ReadOnly))
                    {
                        return {NodeStatus::Failed,
                                QObject::tr("无法读取输入文件：%1").arg(inPath),
                                {}};
                    }
                    if (!outFile.open(QIODevice::WriteOnly))
                    {
                        return {NodeStatus::Failed,
                                QObject::tr("无法写入输出文件：%1").arg(outPath),
                                {}};
                    }
                    // TODO(后端): 替换为甲方 LAS→H5 算法或 EXE func_format_convert，勿保留 readAll 占位
                    outFile.write(inFile.readAll());
                    inFile.close();
                    outFile.close();
                    outs.append(outPath);
                }

                if (onProgress)
                    onProgress(100, QObject::tr("转换完成，共 %1 个 H5").arg(outs.size()));
                return {NodeStatus::Succeeded,
                        QObject::tr("已生成 %1 个 H5 文件").arg(outs.size()),
                        outs};
            }

            QStringList outs;
            for (auto it = m_inputs.constBegin(); it != m_inputs.constEnd(); ++it)
                outs.append(it.value());
            if (onProgress)
                onProgress(100, QStringLiteral("展示节点（无需运行算法）"));
            return {NodeStatus::Succeeded,
                    QStringLiteral("展示节点已就绪"),
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
