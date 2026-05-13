#include "NodeRegistry.h"

namespace processing {

NodeRegistry& NodeRegistry::instance() {
    static NodeRegistry s;
    return s;
}

void NodeRegistry::registerType(const NodeMeta& meta, Creator creator) {
    m_entries.insert(meta.typeId, { meta, std::move(creator) });
}

QList<NodeMeta> NodeRegistry::listAll() const {
    QList<NodeMeta> out;
    out.reserve(m_entries.size());
    for (const auto& e : m_entries) out.push_back(e.meta);
    return out;
}

ProcessNodePtr NodeRegistry::create(const QString& typeId) const {
    auto it = m_entries.constFind(typeId);
    if (it == m_entries.constEnd()) return nullptr;
    auto node = it->creator();
    return node;
}

bool NodeRegistry::hasType(const QString& typeId) const {
    return m_entries.contains(typeId);
}

} // namespace processing
