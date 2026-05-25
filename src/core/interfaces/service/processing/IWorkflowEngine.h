#ifndef I_WORKFLOW_ENGINE_H
#define I_WORKFLOW_ENGINE_H

#include "processing/WorkflowTypes.h"
#include "processing/NodeDefinition.h"
#include "processing/IProcessNode.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointF>
#include <QSizeF>
#include <memory>

namespace processing {

// 节点 / 边的轻量描述（保存到 SQLite + 在 UI 与引擎间传递）
struct NodeInstance {
    QString     instanceId;         // 唯一 ID（引擎生成）
    QString     typeId;             // 节点类型 ID
    QString     displayName;        // 用户重命名后的标题
    QPointF     position;           // 画布坐标
    QSizeF      size;               // 节点拖拽后的宽高（默认 0,0 表示未设置，由 UI 默认值接管）
    QVariantMap params;             // 当前参数
    QStringList inputFiles;         // 用户在 UI 选择的输入路径（仅 PureOutput 节点用）
};

struct EdgeInstance {
    QString fromNode;
    QString fromPort;
    QString toNode;
    QString toPort;
};

/**
 * @brief 工作流引擎抽象接口
 *
 * 职责（PDF 3.1 / 3.2 / 3.3）：
 *   - 维护 DAG（增删节点、增删边、连线校验）
 *   - 三种运行模式：单节点 / 全部自动 / 停止
 *   - 同级无依赖节点并发（QThreadPool）
 *   - 状态变化以信号通知 UI
 *   - 序列化/反序列化（模板=不含数据，工程=含数据）
 *
 * 必须 Q_OBJECT，因为 UI 通过信号槽更新节点状态。
 */
class IWorkflowEngine : public QObject {
    Q_OBJECT
public:
    explicit IWorkflowEngine(QObject* parent = nullptr) : QObject(parent) {}
    ~IWorkflowEngine() override = default;

    // ---------- 拓扑维护 ----------
    virtual QString addNode(const QString& typeId, const QPointF& pos = {}) = 0;
    virtual bool    removeNode(const QString& instanceId) = 0;
    virtual bool    renameNode(const QString& instanceId, const QString& newName) = 0;
    virtual bool    setNodeParams(const QString& instanceId, const QVariantMap& params) = 0;
    // UI 拖拽调整节点位置/尺寸后回写，使之能被序列化
    virtual bool    setNodePosition(const QString& instanceId, const QPointF& pos) = 0;
    virtual bool    setNodeSize    (const QString& instanceId, const QSizeF&  size) = 0;

    // 连线（自动校验上下游格式，失败返回 false 并 emit edgeRejected）
    virtual bool addEdge(const EdgeInstance& edge) = 0;
    virtual bool removeEdge(const EdgeInstance& edge) = 0;

    // 查询
    virtual QList<NodeInstance> nodes() const = 0;
    virtual QList<EdgeInstance> edges() const = 0;
    virtual NodeStatus           statusOf(const QString& instanceId) const = 0;
    virtual QStringList          outputsOf(const QString& instanceId) const = 0;
    /// 解析连到该节点输入端的上游文件（优先用上游运行产出，否则读数据输入节点的 input_files）
    virtual QStringList          upstreamInputFiles(const QString& toNodeId,
                                                    const QString& toPort = {}) const = 0;

    // ---------- 运行控制 ----------
    virtual void runSingle(const QString& instanceId) = 0;   // PDF 3.2.1
    virtual void runAll() = 0;                                // PDF 3.2.2
    virtual void stop() = 0;                                  // PDF 3.2.3
    virtual bool isRunning() const = 0;

    // ---------- 序列化（PDF 3.3） ----------
    // 模板：仅拓扑+参数；工程：含输入/输出文件引用
    virtual QByteArray serialize(bool includeData) const = 0;
    virtual bool       deserialize(const QByteArray& data) = 0;
    virtual void       clear() = 0;

signals:
    // UI 监听这些信号刷新视图
    void nodeAdded(const processing::NodeInstance& node);
    void nodeRemoved(const QString& instanceId);
    void nodeRenamed(const QString& instanceId, const QString& newName);
    void nodeParamsChanged(const QString& instanceId);
    void nodeStatusChanged(const QString& instanceId, processing::NodeStatus status);
    void nodeProgress(const QString& instanceId, int percent, const QString& log);
    void nodeFinished(const QString& instanceId, const processing::NodeRunResult& result);

    void edgeAdded(const processing::EdgeInstance& edge);
    void edgeRemoved(const processing::EdgeInstance& edge);
    void edgeRejected(const processing::EdgeInstance& edge, const QString& reason);

    void runStarted();
    void runStopped();
    void runFinished(bool allSucceeded);
};

} // namespace processing

#endif // I_WORKFLOW_ENGINE_H
