#ifndef DEMO_NODES_H
#define DEMO_NODES_H

#include "service/processing/IProcessNode.h"

#include <QAtomicInteger>
#include <QThread>

namespace processing { namespace demo {

/**
 * @brief 一个最简的 demo 节点基类。
 *
 * 仅做：模拟进度 0→100、把上游 inputFiles 透传到 outputFiles，
 * 不真正调外部进程，方便点 ▶ / 全跑 时看到状态变化。
 */
class DemoNodeBase : public IProcessNode {
public:
    explicit DemoNodeBase(NodeMeta meta) : m_meta(std::move(meta)) {}

    const NodeMeta& meta() const override { return m_meta; }
    QString instanceId() const override { return m_id; }
    void    setInstanceId(const QString& id) override { m_id = id; }

    QVariantMap params() const override { return m_params; }
    void        setParams(const QVariantMap& p) override { m_params = p; }

    void setInputFiles(const QString& portKey, const QStringList& files) override {
        m_inputs[portKey] = files;
    }

    NodeRunResult run(ProgressCallback onProgress) override {
        m_cancel.storeRelaxed(0);
        // 仅用于 UI 演示进度推进。真实节点应以实际计算耗时为准，
        // 不要在生产代码里 sleep。
        const int steps = 5;
        const int stepMs = 60;
        for (int i = 1; i <= steps; ++i) {
            if (m_cancel.loadRelaxed()) {
                return { NodeStatus::Stopped, QStringLiteral("用户停止"), {} };
            }
            QThread::msleep(stepMs);
            if (onProgress) onProgress(i * 100 / steps,
                QStringLiteral("%1 step %2/%3").arg(m_meta.displayName).arg(i).arg(steps));
        }
        // 透传上游路径作为输出（演示用），如果是 PureOutput 节点就用 params 里的路径
        QStringList out;
        for (const auto& v : m_inputs) out += v;
        if (out.isEmpty() && m_params.contains("path")) out << m_params["path"].toString();
        return { NodeStatus::Succeeded, QStringLiteral("ok"), out };
    }

    void requestCancel() override         { m_cancel.storeRelaxed(1); }
    bool isCancelRequested() const override { return m_cancel.loadRelaxed() != 0; }

protected:
    NodeMeta                          m_meta;
    QString                           m_id;
    QVariantMap                       m_params;
    QHash<QString, QStringList>       m_inputs;
    mutable QAtomicInteger<int>       m_cancel{0};
};

/// 注册 4 个 demo 节点到 NodeRegistry。在 ProcessingPage 构造时调用一次。
void registerDemoNodes();

}} // namespace processing::demo

#endif // DEMO_NODES_H
