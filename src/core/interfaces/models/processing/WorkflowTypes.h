#ifndef WORKFLOW_TYPES_H
#define WORKFLOW_TYPES_H

#include <QString>
#include <QStringList>
#include <QMetaType>

namespace processing {

// ============================================================
// 节点分组（PDF 4.1 节点分组）
// ============================================================
enum class NodeGroup {
    DataInput,      // 数据管理类（数据输入）
    Preprocess,     // 预处理类
    Interpret,      // 解释分析类（预留）
    Display         // 展示类
};

// ============================================================
// 节点类型（PDF 4.2 严格控制数据流）
// ============================================================
enum class NodeKind {
    PureOutput,     // 纯输出节点：无上游，仅产出数据（数据输入类）
    InOut,          // 输入输出节点：有上游 → 处理 → 产出（预处理 / 解释类）
    PureInput       // 纯输入节点：有上游，不再产出后续数据（展示 / 报告类）
};

// ============================================================
// 节点运行时状态（PDF 3.2 + 4.3.4 状态指示灯）
// ============================================================
enum class NodeStatus {
    Idle,           // 等待
    Running,        // 运行中
    Succeeded,      // 成功
    Failed,         // 失败
    Stopped,        // 被用户终止
    Error           // 严重错误（如外部进程崩溃）
};

// ============================================================
// 数据格式（用于端口校验，PDF 3.1 自动校验规则）
// 项目内部统一 HDF5；其它格式仅在 DataInput 节点输出
// ============================================================
enum class DataFormat {
    Unknown,
    HDF5,           // 统一内部格式（PDF 5.1.5）
    LAS,
    LTD,
    TDMS,
    SEGY,
    CSV,
    MAT,
    WITS,
    JSON,
    XML,
    WellboreModel,  // 井眼模型
    WellStructure   // 井筒结构
};

QString toString(NodeGroup g);
QString toString(NodeKind k);
QString toString(NodeStatus s);
QString toString(DataFormat f);

DataFormat dataFormatFromString(const QString& s);

} // namespace processing

Q_DECLARE_METATYPE(processing::NodeStatus)
Q_DECLARE_METATYPE(processing::DataFormat)

#endif // WORKFLOW_TYPES_H
