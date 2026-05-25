#ifndef WORKFLOW_ENGINE_H
#define WORKFLOW_ENGINE_H

#include "service/processing/IWorkflowEngine.h"
#include "service/processing/INodeFactory.h"

#include <QHash>
#include <QSet>
#include <QMutex>
#include <QThreadPool>
#include <atomic>
#include <memory>

namespace processing {

/**
 * @brief 默认工作流引擎实现
 *
 * - 用 QHash + 邻接表维护 DAG
 * - 用 QThreadPool 调度同级并发节点
 * - 状态变化通过 Qt 信号回到 UI 线程
 *
 * 注意：本类只负责调度 + 状态机；实际算法在各个 IProcessNode 实现里。
 */
class WorkflowEngine : public IWorkflowEngine {
    Q_OBJECT
public:
    explicit WorkflowEngine(INodeFactory* factory, QObject* parent = nullptr);
    ~WorkflowEngine() override;

    QString addNode(const QString& typeId, const QPointF& pos = {}) override;
    bool    removeNode(const QString& instanceId) override;
    bool    renameNode(const QString& instanceId, const QString& newName) override;
    bool    setNodeParams(const QString& instanceId, const QVariantMap& params) override;
    bool    setNodePosition(const QString& instanceId, const QPointF& pos) override;
    bool    setNodeSize    (const QString& instanceId, const QSizeF&  size) override;

    bool addEdge(const EdgeInstance& edge) override;
    bool removeEdge(const EdgeInstance& edge) override;

    QList<NodeInstance> nodes() const override;
    QList<EdgeInstance> edges() const override;
    NodeStatus           statusOf(const QString& instanceId) const override;
    QStringList          outputsOf(const QString& instanceId) const override;
    QStringList          upstreamInputFiles(const QString& toNodeId,
                                            const QString& toPort = {}) const override;

    void runSingle(const QString& instanceId) override;
    void runAll() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }

    QByteArray serialize(bool includeData) const override;
    bool       deserialize(const QByteArray& data) override;
    void       clear() override;

private:
    /// 引擎内部一条节点记录：UI 快照 + 可执行实例 + 运行状态/产出
    struct NodeRecord {
        NodeInstance   info;       // 与画布、序列化对应的轻量描述
        ProcessNodePtr node;       // 真正执行 run() 的对象（ExternalProcessNode 等）
        NodeStatus     status = NodeStatus::Idle;
        QStringList    outputs;    // 上次 run 成功的 outputFiles，供下游 setInputFiles
    };

    QString generateId() const;
    bool wouldCreateCycle(const QString& from, const QString& to) const;
    bool validateEdge(const EdgeInstance& e, QString* reason) const;

    /// 后台执行单节点；运行前注入上游路径到 m_inputs
    void scheduleNode(const QString& id);
    /// 单个上游节点可提供的路径（outputs 优先，数据输入可回退 params）
    QStringList resolveUpstreamFiles(const NodeRecord& rec) const;
    QSet<QString> ancestors(const QString& id) const;

    QList<QStringList> topoLayers() const;
    void scheduleLayer(int layerIndex);
    void finishRun(bool allSucceeded);
    /// 节点无法执行或已从图中移除时仍发出 nodeFinished，避免 runSingle 卡死
    void notifyNodeAborted(const QString& id, const QString& message);

    INodeFactory* m_factory;
    QHash<QString, NodeRecord> m_records;  // instanceId → 记录
    QList<EdgeInstance> m_edges;        // 邻接表（全图边列表）
    QThreadPool m_pool;                   // scheduleNode 使用的线程池
    std::atomic<bool> m_running{false};   // 是否处于 runSingle/runAll 会话中
    std::atomic<bool> m_stopRequested{false};
    mutable QMutex m_mutex;               // 保护 m_records / m_edges

    // 仅 runAll 使用（主线程读写）
    QList<QStringList> m_runLayers;       // topoLayers 结果，每层可并行
    int m_runCurrentLayer = -1;
    QSet<QString> m_runPending;           // 当前层尚未 nodeFinished 的 id
    bool m_runAllSucceeded = true;
};

} // namespace processing

#endif // WORKFLOW_ENGINE_H
