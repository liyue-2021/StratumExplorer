// =============================================================================
//  ExternalProcessNode.h
//  作者：工程师 ly
//
//  通用外部进程节点：实现 IProcessNode，run() 直接委托给 ExternalProcessRunner。
//  后端交付的每一个算法 EXE 都可以用同一份 ExternalProcessNode 包装，
//  只需提供不同的 NodeMeta 即可在节点库中注册为不同节点。
//
//  EXE 路径获取优先级：
//    1. 节点参数 params["exePath"]（用户可在属性面板修改）
//    2. 后续可接入 ToolRegistry / 全局设置
// =============================================================================
#ifndef EXTERNAL_PROCESS_NODE_H
#define EXTERNAL_PROCESS_NODE_H

#include "service/processing/IProcessNode.h"
#include "ExternalProcessRunner.h"

#include <QAtomicInteger>
#include <QHash>
#include <QStringList>

namespace processing {

/**
 * @brief 通用外部进程节点 —— 一个 NodeMeta 对应一种算法 EXE。
 *
 * 使用方式：
 *   NodeMeta meta = { ... };
 *   NodeRegistry::instance().registerType(meta,
 *       [meta]{ return std::make_shared<ExternalProcessNode>(meta); });
 *
 * run() 内部：
 *   1. 从 params["exePath"] 拿到可执行文件路径
 *   2. 组装 TaskRequest（params / inputs / outputPath）
 *   3. 调用 ExternalProcessRunner::runBlocking()
 *   4. 把 TaskResult 转为 NodeRunResult 返回
 */
class ExternalProcessNode : public IProcessNode {
public:
    explicit ExternalProcessNode(NodeMeta meta);

    // ---- IProcessNode ----
    const NodeMeta& meta() const override;
    QString instanceId() const override;
    void    setInstanceId(const QString& id) override;

    QVariantMap params() const override;
    void        setParams(const QVariantMap& p) override;

    void setInputFiles(const QString& portKey, const QStringList& files) override;

    NodeRunResult run(ProgressCallback onProgress) override;

    void requestCancel() override;
    bool isCancelRequested() const override;

private:
    /// 生成输出文件路径：%TEMP%/oilpro/<instanceId>.h5
    QString buildOutputPath() const;

    NodeMeta                          m_meta;
    QString                           m_id;
    QVariantMap                       m_params;
    QHash<QString, QStringList>       m_inputs;
    ExternalProcessRunner             m_runner;
};

} // namespace processing

#endif // EXTERNAL_PROCESS_NODE_H
