#ifndef NODE_DEFINITION_H
#define NODE_DEFINITION_H

#include "processing/WorkflowTypes.h"

#include <QString>
#include <QVariantMap>
#include <QList>
#include <QMap>

namespace processing
{

    // 端口定义：上下游连接的数据契约
    struct PortSpec
    {
        QString key;         // 端口键（如 "in", "out"）
        QString displayName; // 显示名
        DataFormat format;   // 期望/产出数据格式
    };

    // 节点静态元数据：注册到 NodeRegistry，供 UI 节点库 + 工厂使用
    struct NodeMeta
    {
        QString typeId;              // 全局唯一类型 ID（如 "input.hdf5", "preprocess.bandpass"）
        QString displayName;         // 中文显示名
        QString category;            // 二级分类（如 "DAS 处理", "DTS 处理"）
        int order = 0;               // 节点展示顺序，数字越小排越前
        NodeGroup group;             // 大组
        NodeKind kind;               // 数据流方向
        QList<PortSpec> inputs;      // 输入端口列表（PureOutput 为空）
        QList<PortSpec> outputs;     // 输出端口列表（PureInput 为空）
        QVariantMap defaultParams;   // 默认参数（用户可改 / 重置）
        QString description;         // 节点说明（鼠标悬浮提示）
        bool externalProcess = true; // 是否走外部进程

        // === 以下字段用于 ExternalProcessRunner HDF5 协议 ===
        int funcId = 0;   // 功能编号 0~41（与 ProductionNodes 一致，后端 EXE 按此路由）
        QString funcName; // Python 函数名（如 func_depth_correct）

        // 参数中文标签映射（key -> 中文显示名）
        QMap<QString, QString> paramLabels;

        // 属性面板参数展示顺序（空则按 key 字典序）
        QStringList paramOrder;

        // true：属性参数来自行业通用模板，暂为虚构（见 node_client_params.json fictional）
        bool clientParamsFictional = false;

        // 不在右侧属性面板展示参数（见 node_client_params.json propertyPanel: "none"）
        bool hidePropertyPanel = false;

        // 双击节点时弹出的配置对话框 ID（如 format_convert）
        QString configDialogId;
    };

    // 端口匹配检查（连线时调用，PDF 3.1.3）
    inline bool isCompatible(DataFormat upstreamOut, DataFormat downstreamIn)
    {
        if (downstreamIn == DataFormat::Unknown)
            return true; // 下游接收 any
        if (upstreamOut == DataFormat::Unknown)
            return false;
        return upstreamOut == downstreamIn;
    }

} // namespace processing

#endif // NODE_DEFINITION_H
