#ifndef NODE_COMPATIBILITY_H
#define NODE_COMPATIBILITY_H

#include "models/processing/NodeDefinition.h"
#include "service/processing/IWorkflowEngine.h"

#include <functional>
#include <QString>

namespace processing
{

/**
 * @brief 传感器/业务数据族（甲方《算法节点兼容性校验说明》）
 *
 * DAS / DTS / DSS 三类严禁在同一处理链中混用（除 DSS↔DTS 的温度耦合节点 22 特例）。
 * Neutral 表示「尚未归属某一族」的桥接节点（数据输入、格式转换等）。
 */
enum class SensorFamily
{
    Neutral,
    DAS,
    DTS,
    DSS,
};

/**
 * @brief 算法数据形态层级（甲方文档 A/B/C）
 *
 * - A：时域二维矩阵，同类可级联，可进入 B，不能接收 B/C 的输出
 * - B：频域/时频（仅 DAS 有），B 内可级联，可进入 C，禁止回接 A
 * - C：特征/统计量（链路终点），后面不能再接任何算法
 * - Coupling：22 号温度耦合分离专用
 */
enum class DataTier
{
    None,
    A_TimeDomain,
    B_Spectrum,
    C_Feature,
    Coupling,
};

QString toString(SensorFamily f);
QString toString(DataTier t);

/// 单个节点在兼容性规则中的静态画像（由 typeId 查表得到）
struct NodeCompatProfile
{
    SensorFamily family = SensorFamily::Neutral;
    DataTier tier = DataTier::None;
    NodeGroup group = NodeGroup::Preprocess;

    /// true：C 类终点，禁止再连出到任何算法/展示（展示类除外见代码）
    bool terminal = false;

    /// true：22 号温度耦合，仅 DSS+DTS 配对
    bool couplingNode = false;
};

/// 根据节点 typeId 返回兼容性画像（未知类型按 Neutral/None 处理）
NodeCompatProfile compatProfileForTypeId(const QString &typeId);

/**
 * @brief 判断「上游节点 → 下游节点」在算法语义上是否允许连线
 *
 * 与 WorkflowEngine::validateEdge 配合：在 HDF5 端口格式校验通过后再调用本函数。
 * @param upstream 源节点 meta
 * @param downstream 目标节点 meta
 * @param reason 失败时写入中文说明
 */
bool isAlgorithmEdgeCompatible(const NodeMeta &upstream,
                               const NodeMeta &downstream,
                               QString *reason = nullptr);

/**
 * @brief 全图校验：除 pairwise 边规则外，检查 C 类/终点节点是否仍有出边
 * @param edges 当前所有边
 * @param typeIdOfNode 由 instanceId 查 typeId
 * @param metaOfType 由 typeId 查 NodeMeta
 */
/// 画布临时标注：甲方文档序号 + 数据族/形态（测试用，funcId 与 ProductionNodes 一致）
QString nodeTestSeqLabel(const QString &typeId, int funcId);

bool validateWorkflowCompatGraph(
    const QList<EdgeInstance> &edges,
    const std::function<QString(const QString &instanceId)> &typeIdOfNode,
    const std::function<const NodeMeta *(const QString &typeId)> &metaOfType,
    QString *reason = nullptr);

} // namespace processing

#endif // NODE_COMPATIBILITY_H
