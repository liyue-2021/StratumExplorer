#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include "service/processing/INodeFactory.h"

#include <QHash>
#include <QString>
#include <functional>

namespace processing {

/**
 * @brief 节点类型注册中心（单例式工厂）
 *
 * 用法：
 *   NodeRegistry::instance().registerType(meta, []{ return std::make_shared<MyNode>(); });
 *
 * 新节点接入流程（PDF 6.2 / 7.2）：
 *   1. 实现 IProcessNode
 *   2. 在 service/processing/nodes/Bootstrap.cpp 注册
 *   3. 不需要修改引擎或 UI
 */
class NodeRegistry : public INodeFactory {
public:
    using Creator = std::function<ProcessNodePtr()>;

    static NodeRegistry& instance();

    void registerType(const NodeMeta& meta, Creator creator);

    // INodeFactory
    QList<NodeMeta> listAll() const override;
    ProcessNodePtr  create(const QString& typeId) const override;
    bool            hasType(const QString& typeId) const override;

private:
    NodeRegistry() = default;
    NodeRegistry(const NodeRegistry&) = delete;
    NodeRegistry& operator=(const NodeRegistry&) = delete;

    struct Entry { NodeMeta meta; Creator creator; };
    QHash<QString, Entry> m_entries;
};

} // namespace processing

#endif // NODE_REGISTRY_H
