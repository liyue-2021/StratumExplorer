#ifndef I_PROCESS_NODE_H
#define I_PROCESS_NODE_H

#include "processing/WorkflowTypes.h"
#include "processing/NodeDefinition.h"

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QStringList>
#include <memory>
#include <functional>

namespace processing {

// 节点运行结果
struct NodeRunResult {
    NodeStatus  status = NodeStatus::Failed;
    QString     message;            // 错误信息或摘要
    QStringList outputFiles;        // 产出的 HDF5 / 其它文件路径
};

/**
 * @brief 单个工作节点的运行时接口
 *
 * 每个具体节点（如 BandpassFilter、HDF5Reader、ChartView）都要实现此接口。
 * UI 层不直接持有此对象，而是通过 IWorkflowEngine 调度。
 *
 * 实现要求（PDF 5.2.2）：
 *   - 除「数据格式转换」外，其余预处理节点 externalProcess=true，
 *     通过启动外部算法进程 + stdin 通信完成；主程序不阻塞。
 *   - 节点崩溃必须不影响主程序。
 */
class IProcessNode {
public:
    using ProgressCallback = std::function<void(int /*percent*/, const QString& /*log*/)>;

    virtual ~IProcessNode() = default;

    // 节点静态元数据
    virtual const NodeMeta& meta() const = 0;

    // 实例 ID（工作流内唯一），由引擎注入
    virtual QString instanceId() const = 0;
    virtual void    setInstanceId(const QString& id) = 0;

    // 当前参数（用户可在 UI 修改）
    virtual QVariantMap params() const = 0;
    virtual void        setParams(const QVariantMap& p) = 0;

    // 输入文件（按端口 key 索引），由引擎根据上游输出注入
    virtual void setInputFiles(const QString& portKey, const QStringList& files) = 0;

    // 同步执行（在 QThreadPool 工作线程中调用，可阻塞）
    // 必须支持响应 isCancelRequested() 提前退出
    virtual NodeRunResult run(ProgressCallback onProgress) = 0;

    // 由引擎在用户点击「停止」时调用；实现需在合理时间退出 run()
    virtual void requestCancel() = 0;
    virtual bool isCancelRequested() const = 0;
};

using ProcessNodePtr = std::shared_ptr<IProcessNode>;

} // namespace processing

#endif // I_PROCESS_NODE_H
