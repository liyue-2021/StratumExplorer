// =============================================================================
//  ExternalProcessNode.cpp
//  作者：工程师 ly
//
//  通用外部进程节点的实现。把 IProcessNode::run() 转为
//  ExternalProcessRunner::runBlocking()，屏蔽所有跨进程细节。
// =============================================================================
#include "ExternalProcessNode.h"

#include <QDir>
#include <QStandardPaths>

namespace processing {

ExternalProcessNode::ExternalProcessNode(NodeMeta meta)
    : m_meta(std::move(meta))
{
    // 若 NodeMeta 的 defaultParams 里没有 exePath，自动补一个空占位，
    // 这样属性面板总能显示出这个字段供用户填写。
    if (!m_meta.defaultParams.contains(QStringLiteral("exePath"))) {
        m_meta.defaultParams.insert(QStringLiteral("exePath"), QString());
    }
    m_params = m_meta.defaultParams;
}

const NodeMeta& ExternalProcessNode::meta() const { return m_meta; }

QString ExternalProcessNode::instanceId() const        { return m_id; }
void    ExternalProcessNode::setInstanceId(const QString& id) { m_id = id; }

QVariantMap ExternalProcessNode::params() const        { return m_params; }
void        ExternalProcessNode::setParams(const QVariantMap& p) { m_params = p; }

void ExternalProcessNode::setInputFiles(const QString& portKey,
                                        const QStringList& files)
{
    m_inputs[portKey] = files;
}

NodeRunResult ExternalProcessNode::run(ProgressCallback onProgress)
{
    // ---- 1. 拿 EXE 路径 ----
    const QString exePath = m_params.value(QStringLiteral("exePath")).toString();
    if (exePath.isEmpty()) {
        return { NodeStatus::Failed,
                 QStringLiteral("节点 [%1] 未配置 exePath 参数").arg(m_meta.displayName),
                 {} };
    }

    // ---- 2. 组装 TaskRequest ----
    ExternalProcessRunner::TaskRequest req;
    req.exePath    = exePath;
    req.params     = m_params;
    // exePath 是前端管理字段，不需要发给算法进程
    req.params.remove(QStringLiteral("exePath"));
    req.inputs     = m_inputs;
    req.outputPath = buildOutputPath();

    // ---- 3. 委托给 Runner ----
    ExternalProcessRunner::TaskResult tr = m_runner.runBlocking(req, onProgress);

    // ---- 4. 转换结果 ----
    return { tr.status, tr.message, tr.outputs };
}

void ExternalProcessNode::requestCancel()          { m_runner.requestCancel(); }
bool ExternalProcessNode::isCancelRequested() const { return m_runner.isCancelRequested(); }

QString ExternalProcessNode::buildOutputPath() const
{
    // %TEMP%/oilpro/<instanceId>.h5
    const QString tmpDir = QDir::tempPath() + QStringLiteral("/oilpro");
    QDir().mkpath(tmpDir);
    return tmpDir + QStringLiteral("/") + m_id + QStringLiteral(".h5");
}

} // namespace processing
