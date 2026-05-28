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
#include "ParamRangePair.h"
#ifdef WITH_HDF5
#include "LASToHDF5Converter.h"
#endif

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace processing
{

    ExternalProcessNode::ExternalProcessNode(NodeMeta meta)
        : m_meta(std::move(meta))
    {
        // exePath 由 ProductionNodeParams::applyClientParams 注入 defaultParams，此处不再重复插入
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
        // 格式转换：LAS → HDF5（力炜 LASToHDF5Converter，应用内转换，不启 EXE）
        if (m_meta.typeId.contains(QLatin1String("format_convert"))) {
            const QString outDir = m_params.value(QStringLiteral("output_dir")).toString().trimmed();
            if (outDir.isEmpty()) {
                return {NodeStatus::Failed, QObject::tr("请先在属性面板选择输出路径"), {}};
            }
            QDir dir(outDir);
            if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
                return {NodeStatus::Failed, QObject::tr("无法创建输出目录：%1").arg(outDir), {}};
            }

            QStringList convertOutputs;
            const QString outType =
                m_params.value(QStringLiteral("output_type"), QStringLiteral("h5")).toString();
            for (const QString& inputFilePath : m_inputs.value(QStringLiteral("in"))) {
                const QFileInfo inputFileInfo(inputFilePath);
                if (!inputFileInfo.exists()) {
                    return {NodeStatus::Failed,
                            QObject::tr("输入文件不存在: %1").arg(inputFilePath),
                            {}};
                }

                const QString outputFileName =
                    inputFileInfo.completeBaseName() + QLatin1Char('.') + outType;
                const QString outputFilePath = dir.filePath(outputFileName);

                if (inputFileInfo.suffix().compare(outType, Qt::CaseInsensitive) == 0) {
                    convertOutputs.append(inputFilePath);
                    continue;
                }

                if (inputFileInfo.suffix().compare(QLatin1String("las"), Qt::CaseInsensitive) == 0
                    && outType.compare(QLatin1String("h5"), Qt::CaseInsensitive) == 0) {
#ifdef WITH_HDF5
                    LASToHDF5Converter converter;
                    if (!converter.convertLASFile(inputFilePath, outputFilePath)) {
                        return {NodeStatus::Failed,
                                QObject::tr("LAS 转 HDF5 失败: %1").arg(inputFilePath),
                                {}};
                    }
                    convertOutputs.append(outputFilePath);
                    continue;
#else
                    return {NodeStatus::Failed,
                            QObject::tr("LAS→HDF5 需要 WITH_HDF5=ON 构建"),
                            {}};
#endif
                }

                return {NodeStatus::Failed,
                        QObject::tr("暂不支持 %1 → %2 转换")
                            .arg(inputFileInfo.suffix(), outType),
                        {}};
            }

            if (onProgress)
                onProgress(100, QObject::tr("已转换 %1 个文件").arg(convertOutputs.size()));
            return {NodeStatus::Succeeded,
                    QObject::tr("已转换 %1 个文件").arg(convertOutputs.size()),
                    convertOutputs};
        }

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

        // 范围对 min/max → legacy 数组字段（兼容后端 HDF5 协议）
        syncLegacyRangeArrays(req.params, paramRangePairsForTypeId(m_meta.typeId));

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
