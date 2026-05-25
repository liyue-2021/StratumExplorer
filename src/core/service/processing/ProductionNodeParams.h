#ifndef PRODUCTION_NODE_PARAMS_H
#define PRODUCTION_NODE_PARAMS_H

#include "processing/NodeDefinition.h"

namespace processing::production
{

    // 甲方节点属性参数（node_client_params.json）
    //
    // 甲方已确认参数（示例）：preprocess.depth_correct / depth_time_correct / data_crop
    // 其余预处理节点仍可能为旧 Excel 模板字段，见各节点 note
    //
    // 暂为虚构（19 个，行业通用模板，保留面板占位，待甲方确认后替换）：
    //   preprocess.format_convert, preprocess.phys_convert
    //   interpret.* 共 12 个
    //   display.* 共 5 个

    void applyClientParams(NodeMeta &meta);

    NodeMeta finalizeMeta(NodeMeta meta);

    QVariantMap mergeWithClientDefaults(const QString &typeId, const QVariantMap &saved);

    bool isFictionalClientParams(const QString &typeId);

} // namespace processing::production

#endif // PRODUCTION_NODE_PARAMS_H
