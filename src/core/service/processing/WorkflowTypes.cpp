#include "processing/WorkflowTypes.h"

namespace processing {

QString toString(NodeGroup g) {
    switch (g) {
        case NodeGroup::DataInput:  return QStringLiteral("数据管理");
        case NodeGroup::Preprocess: return QStringLiteral("预处理");
        case NodeGroup::Interpret:  return QStringLiteral("解释分析");
        case NodeGroup::Display:    return QStringLiteral("展示");
    }
    return {};
}

QString toString(NodeKind k) {
    switch (k) {
        case NodeKind::PureOutput: return QStringLiteral("纯输出");
        case NodeKind::InOut:      return QStringLiteral("输入输出");
        case NodeKind::PureInput:  return QStringLiteral("纯输入");
    }
    return {};
}

QString toString(NodeStatus s) {
    switch (s) {
        case NodeStatus::Idle:      return QStringLiteral("等待");
        case NodeStatus::Running:   return QStringLiteral("运行中");
        case NodeStatus::Succeeded: return QStringLiteral("成功");
        case NodeStatus::Failed:    return QStringLiteral("失败");
        case NodeStatus::Stopped:   return QStringLiteral("已停止");
        case NodeStatus::Error:     return QStringLiteral("错误");
    }
    return {};
}

QString toString(DataFormat f) {
    switch (f) {
        case DataFormat::Unknown:        return QStringLiteral("Unknown");
        case DataFormat::HDF5:           return QStringLiteral("HDF5");
        case DataFormat::LAS:            return QStringLiteral("LAS");
        case DataFormat::LTD:            return QStringLiteral("LTD");
        case DataFormat::TDMS:           return QStringLiteral("TDMS");
        case DataFormat::SEGY:           return QStringLiteral("SEG-Y");
        case DataFormat::CSV:            return QStringLiteral("CSV");
        case DataFormat::MAT:            return QStringLiteral("MAT");
        case DataFormat::WITS:           return QStringLiteral("WITS");
        case DataFormat::JSON:           return QStringLiteral("JSON");
        case DataFormat::XML:            return QStringLiteral("XML");
        case DataFormat::WellboreModel:  return QStringLiteral("井眼模型");
        case DataFormat::WellStructure:  return QStringLiteral("井筒结构");
    }
    return {};
}

DataFormat dataFormatFromString(const QString& s) {
    const auto u = s.toUpper();
    if (u == "HDF5")  return DataFormat::HDF5;
    if (u == "LAS")   return DataFormat::LAS;
    if (u == "LTD")   return DataFormat::LTD;
    if (u == "TDMS")  return DataFormat::TDMS;
    if (u == "SEG-Y" || u == "SEGY") return DataFormat::SEGY;
    if (u == "CSV")   return DataFormat::CSV;
    if (u == "MAT")   return DataFormat::MAT;
    if (u == "WITS")  return DataFormat::WITS;
    if (u == "JSON")  return DataFormat::JSON;
    if (u == "XML")   return DataFormat::XML;
    return DataFormat::Unknown;
}

} // namespace processing
