#ifndef I_NODE_FACTORY_H
#define I_NODE_FACTORY_H

#include "processing/IProcessNode.h"
#include "processing/NodeDefinition.h"

#include <QString>
#include <QList>
#include <memory>

namespace processing {

/**
 * @brief 节点工厂 / 注册表接口
 *
 * 新增节点 → 实现 IProcessNode + 调用 registerType() 注册即可，
 * 工作流引擎与 UI 节点库通过 typeId 创建实例（PDF 6.2 可扩展性）。
 */
class INodeFactory {
public:
    virtual ~INodeFactory() = default;

    // 列出所有已注册节点元数据（用于左侧节点库 UI）
    virtual QList<NodeMeta> listAll() const = 0;

    // 按 typeId 创建一个新实例（不分配 instanceId，由调用者赋）
    virtual ProcessNodePtr create(const QString& typeId) const = 0;

    // 查询元数据
    virtual bool hasType(const QString& typeId) const = 0;
};

} // namespace processing

#endif // I_NODE_FACTORY_H
