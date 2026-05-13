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

    void runSingle(const QString& instanceId) override;
    void runAll() override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }

    QByteArray serialize(bool includeData) const override;
    bool       deserialize(const QByteArray& data) override;
    void       clear() override;

private:
    struct NodeRecord {
        NodeInstance   info;
        ProcessNodePtr node;             // 运行时实例（lazy 创建）
        NodeStatus     status = NodeStatus::Idle;
        QStringList    outputs;          // 最近一次成功产出
    };

    // 工具
    QString  generateId() const;
    bool     wouldCreateCycle(const QString& from, const QString& to) const;
    bool     validateEdge(const EdgeInstance& e, QString* reason) const;
    void     scheduleNode(const QString& id);
    QSet<QString> ancestors(const QString& id) const;

    // runAll 拓扑调度：同层并发 + 全部节点完成后 emit runFinished
    QList<QStringList> topoLayers() const;          // 分层后的节点 ID
    void     scheduleLayer(int layerIndex);          // 调度第 N 层，全部完成后推进下一层
    void     finishRun(bool allSucceeded);           // 释放运行锁 + emit runFinished

    INodeFactory*                m_factory;
    QHash<QString, NodeRecord>   m_records;
    QList<EdgeInstance>          m_edges;
    QThreadPool                  m_pool;
    std::atomic<bool>            m_running{false};
    std::atomic<bool>            m_stopRequested{false};
    mutable QMutex               m_mutex;

    // runAll 运行上下文（仅在主线程访问）
    QList<QStringList>           m_runLayers;
    int                          m_runCurrentLayer = -1;
    QSet<QString>                m_runPending;       // 当前层未完成的节点
    bool                         m_runAllSucceeded = true;
};

} // namespace processing

#endif // WORKFLOW_ENGINE_H
