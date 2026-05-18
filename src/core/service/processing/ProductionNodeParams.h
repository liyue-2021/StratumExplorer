#ifndef PRODUCTION_NODE_PARAMS_H
#define PRODUCTION_NODE_PARAMS_H

#include "processing/NodeDefinition.h"

namespace processing::production
{

    // 甲方节点属性参数（node_client_params.json）
    //
    // 真实参数（23 个，来自 Excel，可对接后端）：
    //   preprocess.depth_correct ~ preprocess.baseline_calib（不含 format_convert、phys_convert）
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
