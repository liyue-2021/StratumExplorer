#ifndef NODE_DEFINITION_H
#define NODE_DEFINITION_H

#include "processing/WorkflowTypes.h"

#include <QString>
#include <QVariantMap>
#include <QList>

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
        bool externalProcess = true; // 是否走 stdin 跨进程；
                                     // 数据格式转换=false（PDF 5.2.2）
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
