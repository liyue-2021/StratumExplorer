#include "WorkflowEngine.h"
#include "LogBus.h"
#include "ProductionNodeParams.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <QMutexLocker>

namespace processing {

WorkflowEngine::WorkflowEngine(INodeFactory* factory, QObject* parent)
    : IWorkflowEngine(parent), m_factory(factory) {
    // 同级并发上限（可按机器核数调整）
    m_pool.setMaxThreadCount(qMax(2, QThread::idealThreadCount()));
}

WorkflowEngine::~WorkflowEngine() {
    stop();
    m_pool.waitForDone();
}

// ---------------------------------------------------------------- 拓扑

QString WorkflowEngine::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString WorkflowEngine::addNode(const QString& typeId, const QPointF& pos) {
    if (!m_factory || !m_factory->hasType(typeId)) return {};
    auto node = m_factory->create(typeId);
    if (!node) return {};

    NodeRecord rec;
    rec.info.instanceId  = generateId();
    rec.info.typeId      = typeId;
    rec.info.displayName = node->meta().displayName;
    rec.info.position    = pos;
    rec.info.params =
        production::mergeWithClientDefaults(typeId, node->meta().defaultParams);
    rec.node = node;
    node->setInstanceId(rec.info.instanceId);
    node->setParams(rec.info.params);

    {
        QMutexLocker lock(&m_mutex);
        m_records.insert(rec.info.instanceId, rec);
    }
    emit nodeAdded(rec.info);
    return rec.info.instanceId;
}

bool WorkflowEngine::removeNode(const QString& id) {
    {
        QMutexLocker lock(&m_mutex);
        if (!m_records.remove(id)) return false;
        // 清理相关边
        m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
            [&](const EdgeInstance& e){ return e.fromNode == id || e.toNode == id; }),
            m_edges.end());
    }
    emit nodeRemoved(id);
    return true;
}

bool WorkflowEngine::renameNode(const QString& id, const QString& name) {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end()) return false;
    it->info.displayName = name;
    lock.unlock();
    emit nodeRenamed(id, name);
    return true;
}

bool WorkflowEngine::setNodeParams(const QString& id, const QVariantMap& params) {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end()) return false;
    it->info.params = params;
    if (it->node) it->node->setParams(params);
    lock.unlock();
    emit nodeParamsChanged(id);
    return true;
}

// 位置/尺寸仅是画布序列化所需，不会影响调度。为避免拖动过程中
// 频繁 emit 产生信号风暴，这里不 emit 专门信号；UI 本就是变化的发起方。
bool WorkflowEngine::setNodePosition(const QString& id, const QPointF& pos) {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end()) return false;
    it->info.position = pos;
    return true;
}

bool WorkflowEngine::setNodeSize(const QString& id, const QSizeF& size) {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end()) return false;
    it->info.size = size;
    return true;
}

// ---------------------------------------------------------------- 边

bool WorkflowEngine::wouldCreateCycle(const QString& from, const QString& to) const {
    // BFS 从 to 出发，看能否到 from
    QSet<QString> visited;
    QList<QString> q{to};
    while (!q.isEmpty()) {
        const auto cur = q.takeFirst();
        if (cur == from) return true;
        if (visited.contains(cur)) continue;
        visited.insert(cur);
        for (const auto& e : m_edges)
            if (e.fromNode == cur) q.append(e.toNode);
    }
    return false;
}

bool WorkflowEngine::validateEdge(const EdgeInstance& e, QString* reason) const {
    auto from = m_records.constFind(e.fromNode);
    auto to   = m_records.constFind(e.toNode);
    if (from == m_records.constEnd() || to == m_records.constEnd()) {
        if (reason) *reason = QStringLiteral("节点不存在");
        return false;
    }

    auto findPort = [](const QList<PortSpec>& ports, const QString& key) -> const PortSpec* {
        for (const auto& p : ports) if (p.key == key) return &p;
        return nullptr;
    };
    const PortSpec* op = findPort(from->node->meta().outputs, e.fromPort);
    const PortSpec* ip = findPort(to->node->meta().inputs,   e.toPort);
    if (!op || !ip) { if (reason) *reason = QStringLiteral("端口不存在"); return false; }
    if (!isCompatible(op->format, ip->format)) {
        if (reason) *reason = QStringLiteral("数据格式不匹配：%1 → %2")
            .arg(toString(op->format), toString(ip->format));
        return false;
    }
    if (wouldCreateCycle(e.fromNode, e.toNode)) {
        if (reason) *reason = QStringLiteral("会形成环路");
        return false;
    }
    return true;
}

bool WorkflowEngine::addEdge(const EdgeInstance& edge) {
    QString reason;
    {
        QMutexLocker lock(&m_mutex);
        if (!validateEdge(edge, &reason)) {
            lock.unlock();
            emit edgeRejected(edge, reason);
            return false;
        }
        m_edges.append(edge);
    }
    emit edgeAdded(edge);
    return true;
}

bool WorkflowEngine::removeEdge(const EdgeInstance& edge) {
    {
        QMutexLocker lock(&m_mutex);
        auto before = m_edges.size();
        m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(), [&](const EdgeInstance& e){
            return e.fromNode == edge.fromNode && e.toNode == edge.toNode
                && e.fromPort == edge.fromPort && e.toPort == edge.toPort;
        }), m_edges.end());
        if (m_edges.size() == before) return false;
    }
    emit edgeRemoved(edge);
    return true;
}

// ---------------------------------------------------------------- 查询

QList<NodeInstance> WorkflowEngine::nodes() const {
    QMutexLocker lock(&m_mutex);
    QList<NodeInstance> out;
    out.reserve(m_records.size());
    for (const auto& r : m_records) out.append(r.info);
    return out;
}

QList<EdgeInstance> WorkflowEngine::edges() const {
    QMutexLocker lock(&m_mutex);
    return m_edges;
}

NodeStatus WorkflowEngine::statusOf(const QString& id) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.constFind(id);
    return it == m_records.constEnd() ? NodeStatus::Idle : it->status;
}

QStringList WorkflowEngine::outputsOf(const QString& id) const {
    QMutexLocker lock(&m_mutex);
    auto it = m_records.constFind(id);
    return it == m_records.constEnd() ? QStringList{} : it->outputs;
}

// ---------------------------------------------------------------- 调度

QSet<QString> WorkflowEngine::ancestors(const QString& id) const {
    QSet<QString> out;
    QList<QString> q{id};
    while (!q.isEmpty()) {
        auto cur = q.takeFirst();
        for (const auto& e : m_edges) {
            if (e.toNode == cur && !out.contains(e.fromNode)) {
                out.insert(e.fromNode);
                q.append(e.fromNode);
            }
        }
    }
    return out;
}

void WorkflowEngine::scheduleNode(const QString& id) {
    NodeRecord* rec = nullptr;
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_records.find(id);
        if (it == m_records.end()) return;
        rec = &it.value();
        rec->status = NodeStatus::Running;
    }
    emit nodeStatusChanged(id, NodeStatus::Running);

    // 注入上游输出文件
    {
        QMutexLocker lock(&m_mutex);
        for (const auto& e : m_edges) {
            if (e.toNode == id) {
                auto up = m_records.constFind(e.fromNode);
                if (up != m_records.constEnd()) {
                    rec->node->setInputFiles(e.toPort, up->outputs);
                }
            }
        }
    }

    auto self = this;
    QtConcurrent::run(&m_pool, [self, id]() {
        ProcessNodePtr node;
        {
            QMutexLocker lock(&self->m_mutex);
            auto it = self->m_records.find(id);
            if (it == self->m_records.end()) return;
            node = it->node;
        }
        auto result = node->run([self, id](int p, const QString& log) {
            emit self->nodeProgress(id, p, log);
            // 同时广播到全局日志总线，LogDock 会收到
            processing::LogBus::instance().post(
                processing::LogLevel::Info,
                id,
                p >= 0 ? QStringLiteral("[%1%] %2").arg(p).arg(log) : log);
        });
        {
            QMutexLocker lock(&self->m_mutex);
            auto it = self->m_records.find(id);
            if (it == self->m_records.end()) return;
            it->status  = result.status;
            it->outputs = result.outputFiles;
        }
        emit self->nodeStatusChanged(id, result.status);
        emit self->nodeFinished(id, result);
        // 状态变迁也走 LogBus，方便以后调试
        processing::LogBus::instance().post(
            result.status == NodeStatus::Failed ? processing::LogLevel::Error
                                                 : processing::LogLevel::Info,
            id,
            QStringLiteral("finished: %1 %2")
                .arg(toString(result.status), result.message));
    });
}

void WorkflowEngine::runSingle(const QString& id) {
    if (m_running.exchange(true)) return;
    m_stopRequested.store(false);
    emit runStarted();

    // 单节点模式：用一次性 lambda 监听本节点完成 → 释放锁
    QMetaObject::Connection* connHolder = new QMetaObject::Connection;
    *connHolder = connect(this, &IWorkflowEngine::nodeFinished, this,
        [this, id, connHolder](const QString& finishedId, const NodeRunResult& r) {
            if (finishedId != id) return;
            QObject::disconnect(*connHolder);
            delete connHolder;
            finishRun(r.status == NodeStatus::Succeeded);
        });
    scheduleNode(id);
}

void WorkflowEngine::runAll() {
    if (m_running.exchange(true)) return;
    m_stopRequested.store(false);
    emit runStarted();

    m_runLayers       = topoLayers();
    m_runAllSucceeded = true;
    m_runCurrentLayer = -1;

    if (m_runLayers.isEmpty()) {
        finishRun(true);
        return;
    }
    scheduleLayer(0);
}

void WorkflowEngine::stop() {
    m_stopRequested.store(true);
    {
        QMutexLocker lock(&m_mutex);
        for (auto& r : m_records) {
            if (r.node) r.node->requestCancel();
        }
    }
    if (m_running.load()) {
        emit runStopped();
        finishRun(false);
    }
}

// ---------------------------------------------------------------- runAll 调度

void WorkflowEngine::finishRun(bool allSucceeded) {
    if (!m_running.exchange(false)) return;     // 已经被 stop / finish 处理过
    m_runLayers.clear();
    m_runPending.clear();
    m_runCurrentLayer = -1;
    emit runFinished(allSucceeded);
}

// 按拓扑分层：第 0 层是入度为 0 的节点；后续每层是"上游全部在前几层"的节点。
QList<QStringList> WorkflowEngine::topoLayers() const {
    QMutexLocker lock(&m_mutex);
    QHash<QString, int> indeg;
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); ++it)
        indeg.insert(it.key(), 0);
    for (const auto& e : m_edges) {
        if (indeg.contains(e.toNode)) indeg[e.toNode] += 1;
    }
    QList<QStringList> layers;
    QSet<QString> done;
    while (done.size() < indeg.size()) {
        QStringList cur;
        for (auto it = indeg.constBegin(); it != indeg.constEnd(); ++it) {
            if (done.contains(it.key())) continue;
            // 上游必须全部 done
            bool ready = true;
            for (const auto& e : m_edges) {
                if (e.toNode == it.key() && !done.contains(e.fromNode)) {
                    ready = false; break;
                }
            }
            if (ready) cur.append(it.key());
        }
        if (cur.isEmpty()) break;       // 出现环（理论上 addEdge 已拦截）
        std::sort(cur.begin(), cur.end());
        layers.append(cur);
        for (const auto& id : cur) done.insert(id);
    }
    return layers;
}

void WorkflowEngine::scheduleLayer(int layerIndex) {
    m_runCurrentLayer = layerIndex;
    if (layerIndex < 0 || layerIndex >= m_runLayers.size()) {
        finishRun(m_runAllSucceeded);
        return;
    }
    if (m_stopRequested.load()) {
        finishRun(false);
        return;
    }
    const QStringList layer = m_runLayers.at(layerIndex);
    m_runPending = QSet<QString>(layer.begin(), layer.end());

    // 先订阅本层所有节点的 finished —— 任一完成则从 pending 移除；全部完成推进下一层
    QMetaObject::Connection* connHolder = new QMetaObject::Connection;
    *connHolder = connect(this, &IWorkflowEngine::nodeFinished, this,
        [this, layerIndex, connHolder](const QString& id, const NodeRunResult& r) {
            if (!m_runPending.contains(id)) return;
            m_runPending.remove(id);
            if (r.status != NodeStatus::Succeeded) m_runAllSucceeded = false;
            if (m_runPending.isEmpty()) {
                QObject::disconnect(*connHolder);
                delete connHolder;
                scheduleLayer(layerIndex + 1);
            }
        });

    for (const auto& id : layer) scheduleNode(id);
}

// ---------------------------------------------------------------- 序列化

QByteArray WorkflowEngine::serialize(bool includeData) const {
    QMutexLocker lock(&m_mutex);
    QJsonObject root;
    QJsonArray ns;
    for (const auto& r : m_records) {
        QJsonObject n;
        n["id"]     = r.info.instanceId;
        n["type"]   = r.info.typeId;
        n["name"]   = r.info.displayName;
        n["x"]      = r.info.position.x();
        n["y"]      = r.info.position.y();
        if (r.info.size.isValid()) {
            n["w"] = r.info.size.width();
            n["h"] = r.info.size.height();
        }
        n["params"] = QJsonObject::fromVariantMap(r.info.params);
        if (includeData) {
            QJsonArray inputs;
            for (const auto& f : r.info.inputFiles) inputs.append(f);
            n["inputs"]  = inputs;
            QJsonArray outs;
            for (const auto& f : r.outputs) outs.append(f);
            n["outputs"] = outs;
        }
        ns.append(n);
    }
    QJsonArray es;
    for (const auto& e : m_edges) {
        QJsonObject je;
        je["from"]     = e.fromNode;
        je["fromPort"] = e.fromPort;
        je["to"]       = e.toNode;
        je["toPort"]   = e.toPort;
        es.append(je);
    }
    root["nodes"] = ns;
    root["edges"] = es;
    root["withData"] = includeData;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool WorkflowEngine::deserialize(const QByteArray& data) {
    auto doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;
    clear();

    const auto root  = doc.object();
    const auto withData = root.value("withData").toBool();

    for (const auto& v : root.value("nodes").toArray()) {
        const auto n = v.toObject();
        const auto type = n.value("type").toString();
        if (!m_factory || !m_factory->hasType(type)) continue;
        auto node = m_factory->create(type);
        NodeRecord rec;
        rec.info.instanceId  = n.value("id").toString();
        rec.info.typeId      = type;
        rec.info.displayName = n.value("name").toString();
        rec.info.position    = QPointF(n.value("x").toDouble(), n.value("y").toDouble());
        if (n.contains("w") && n.contains("h")) {
            rec.info.size = QSizeF(n.value("w").toDouble(), n.value("h").toDouble());
        }
        rec.info.params = production::mergeWithClientDefaults(
            type, n.value("params").toObject().toVariantMap());
        if (withData) {
            for (const auto& f : n.value("inputs").toArray()) rec.info.inputFiles.append(f.toString());
            for (const auto& f : n.value("outputs").toArray()) rec.outputs.append(f.toString());
        }
        node->setInstanceId(rec.info.instanceId);
        node->setParams(rec.info.params);
        rec.node = node;
        {
            QMutexLocker lock(&m_mutex);
            m_records.insert(rec.info.instanceId, rec);
        }
        emit nodeAdded(rec.info);
    }

    for (const auto& v : root.value("edges").toArray()) {
        const auto je = v.toObject();
        EdgeInstance e{
            je.value("from").toString(),
            je.value("fromPort").toString(),
            je.value("to").toString(),
            je.value("toPort").toString()
        };
        {
            QMutexLocker lock(&m_mutex);
            m_edges.append(e);
        }
        emit edgeAdded(e);
    }
    return true;
}

void WorkflowEngine::clear() {
    QList<QString> ids;
    QList<EdgeInstance> es;
    {
        QMutexLocker lock(&m_mutex);
        ids = m_records.keys();
        es  = m_edges;
        m_records.clear();
        m_edges.clear();
    }
    for (const auto& e : es) emit edgeRemoved(e);
    for (const auto& id : ids) emit nodeRemoved(id);
}

} // namespace processing
