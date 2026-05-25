// =============================================================================
//  WorkflowEngine.cpp
//
//  工作流 DAG 引擎：维护节点/连线、校验端口、调度运行、序列化预设。
//
//  职责划分（读代码时先记住这三层）：
//    1. 本文件：拓扑 + 调度 + 把上游文件路径注入下游节点的 m_inputs
//    2. IProcessNode（如 ExternalProcessNode）：真正执行算法，读 m_inputs / m_params
//    3. UI（WorkflowEditorTab / WorkflowScene）：画布、属性面板，通过信号刷新
//
//  数据流（文件路径如何传到格式转换）：
//    数据输入 params["input_files"] → run 后写入 NodeRecord::outputs
//    → scheduleNode 时 resolveUpstreamFiles → setInputFiles(toPort, paths)
//    → 下游节点 m_inputs["in"]，run() 里合并使用
// =============================================================================
#include "WorkflowEngine.h"
#include "LogBus.h"
#include "ProductionNodeParams.h"

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <QMutexLocker>
#include <QDebug>
#include <QDir>

namespace processing {

namespace {

/// 把属性面板存的 input_files（QVariantList）转成 QStringList 路径。
QStringList pathsFromInputFilesParam(const QVariant& filesVar)
{
    QStringList paths;
    if (filesVar.typeId() != QMetaType::QVariantList)
        return paths;
    for (const QVariant& item : filesVar.toList())
    {
        const QString path = QDir::toNativeSeparators(item.toString().trimmed());
        if (!path.isEmpty())
            paths.append(path);
    }
    return paths;
}

} // namespace

// -----------------------------------------------------------------------------
// 构造 / 析构
// -----------------------------------------------------------------------------

WorkflowEngine::WorkflowEngine(INodeFactory* factory, QObject* parent)
    : IWorkflowEngine(parent), m_factory(factory)
{
    // 线程池用于 QtConcurrent::run；同层节点可并行，上限跟 CPU 核数走
    m_pool.setMaxThreadCount(qMax(2, QThread::idealThreadCount()));
}

WorkflowEngine::~WorkflowEngine()
{
    stop();
    m_pool.waitForDone(); // 等后台 run() 结束，避免析构后仍访问 this
}

// -----------------------------------------------------------------------------
// 拓扑：节点
// -----------------------------------------------------------------------------

/// 生成画布上唯一的实例 ID（UUID 无花括号）。
QString WorkflowEngine::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

/**
 * @brief 在画布上新增一个节点实例。
 * @param typeId 节点类型，如 input.data_input、preprocess.format_convert
 * @param pos    画布坐标，供序列化/还原
 * @return 新实例 ID；失败返回空字符串
 */
QString WorkflowEngine::addNode(const QString& typeId, const QPointF& pos)
{
    if (!m_factory || !m_factory->hasType(typeId))
        return {};
    auto node = m_factory->create(typeId);
    if (!node)
        return {};

    NodeRecord rec;
    rec.info.instanceId = generateId();
    rec.info.typeId = typeId;
    rec.info.displayName = node->meta().displayName;
    rec.info.position = pos;
    // 与 node_client_params.json 合并默认参数，避免属性面板缺字段
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

/**
 * @brief 删除节点，并移除所有以该节点为端点的连线。
 */
bool WorkflowEngine::removeNode(const QString& id)
{
    {
        QMutexLocker lock(&m_mutex);
        if (!m_records.remove(id))
            return false;
        m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
                                    [&](const EdgeInstance& e)
                                    {
                                        return e.fromNode == id || e.toNode == id;
                                    }),
                        m_edges.end());
    }
    emit nodeRemoved(id);
    return true;
}

/// 用户重命名节点标题（仅 displayName，不改 typeId）。
bool WorkflowEngine::renameNode(const QString& id, const QString& name)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end())
        return false;
    it->info.displayName = name;
    lock.unlock();
    emit nodeRenamed(id, name);
    return true;
}

/**
 * @brief 更新节点参数（属性面板编辑后调用）。
 *
 * 同步写入 NodeRecord::info.params 与 IProcessNode::m_params。
 * 数据输入若改了 LAS 列表，清空 outputs，避免下游仍用旧路径。
 */
bool WorkflowEngine::setNodeParams(const QString& id, const QVariantMap& params)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end())
        return false;
    it->info.params = params;
    if (it->node)
        it->node->setParams(params);
    if (it->info.typeId == QLatin1String("input.data_input"))
    {
        it->outputs.clear();
        it->status = NodeStatus::Idle;
    }
    lock.unlock();
    emit nodeParamsChanged(id);
    return true;
}

// 位置/尺寸只用于保存预设 JSON，不参与 run 逻辑；拖动时不 emit，避免信号风暴。
bool WorkflowEngine::setNodePosition(const QString& id, const QPointF& pos)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end())
        return false;
    it->info.position = pos;
    return true;
}

bool WorkflowEngine::setNodeSize(const QString& id, const QSizeF& size)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.find(id);
    if (it == m_records.end())
        return false;
    it->info.size = size;
    return true;
}

// -----------------------------------------------------------------------------
// 拓扑：连线
// -----------------------------------------------------------------------------

/**
 * @brief 检测新增边 from→to 是否会形成有向环。
 *
 * 从 to 沿已有边正向 BFS：若能回到 from，则 from→to 会成环。
 * （工作流数据沿 from 输出 → to 输入 流动。）
 */
bool WorkflowEngine::wouldCreateCycle(const QString& from, const QString& to) const
{
    QSet<QString> visited;
    QList<QString> q{to};
    while (!q.isEmpty())
    {
        const auto cur = q.takeFirst();
        if (cur == from)
            return true;
        if (visited.contains(cur))
            continue;
        visited.insert(cur);
        for (const auto& e : m_edges)
            if (e.fromNode == cur)
                q.append(e.toNode);
    }
    return false;
}

/**
 * @brief 连线合法性：两端节点存在、端口存在、数据格式兼容、不成环。
 * @param reason 失败时写入中文原因，供 UI 显示
 */
bool WorkflowEngine::validateEdge(const EdgeInstance& e, QString* reason) const
{
    auto from = m_records.constFind(e.fromNode);
    auto to = m_records.constFind(e.toNode);
    if (from == m_records.constEnd() || to == m_records.constEnd())
    {
        if (reason)
            *reason = QStringLiteral("节点不存在");
        return false;
    }

    auto findPort = [](const QList<PortSpec>& ports, const QString& key) -> const PortSpec*
    {
        for (const auto& p : ports)
            if (p.key == key)
                return &p;
        return nullptr;
    };
    const PortSpec* op = findPort(from->node->meta().outputs, e.fromPort);
    const PortSpec* ip = findPort(to->node->meta().inputs, e.toPort);
    if (!op || !ip)
    {
        if (reason)
            *reason = QStringLiteral("端口不存在");
        return false;
    }
    // 下游端口为 Unknown 表示可接任意上游格式（如部分预处理 in）
    if (!isCompatible(op->format, ip->format))
    {
        if (reason)
            *reason = QStringLiteral("数据格式不匹配：%1 → %2")
                          .arg(toString(op->format), toString(ip->format));
        return false;
    }
    if (wouldCreateCycle(e.fromNode, e.toNode))
    {
        if (reason)
            *reason = QStringLiteral("会形成环路");
        return false;
    }
    return true;
}

/// 校验通过后追加边，并通知画布画连线。
bool WorkflowEngine::addEdge(const EdgeInstance& edge)
{
    QString reason;
    {
        QMutexLocker lock(&m_mutex);
        if (!validateEdge(edge, &reason))
        {
            lock.unlock();
            emit edgeRejected(edge, reason);
            return false;
        }
        m_edges.append(edge);
    }
    emit edgeAdded(edge);
    return true;
}

/// 删除一条边（from/to 及端口四元组完全匹配才删）。
bool WorkflowEngine::removeEdge(const EdgeInstance& edge)
{
    {
        QMutexLocker lock(&m_mutex);
        const auto before = m_edges.size();
        m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
                                     [&](const EdgeInstance& e)
                                     {
                                         return e.fromNode == edge.fromNode
                                                && e.toNode == edge.toNode
                                                && e.fromPort == edge.fromPort
                                                && e.toPort == edge.toPort;
                                     }),
                        m_edges.end());
        if (m_edges.size() == before)
            return false;
    }
    emit edgeRemoved(edge);
    return true;
}

// -----------------------------------------------------------------------------
// 查询（UI / 属性面板只读）
// -----------------------------------------------------------------------------

/// 返回所有节点快照（不含运行时 node 指针，仅 NodeInstance）。
QList<NodeInstance> WorkflowEngine::nodes() const
{
    QMutexLocker lock(&m_mutex);
    QList<NodeInstance> out;
    out.reserve(m_records.size());
    for (const auto& r : m_records)
        out.append(r.info);
    return out;
}

QList<EdgeInstance> WorkflowEngine::edges() const
{
    QMutexLocker lock(&m_mutex);
    return m_edges;
}

NodeStatus WorkflowEngine::statusOf(const QString& id) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.constFind(id);
    return it == m_records.constEnd() ? NodeStatus::Idle : it->status;
}

/// 某节点最近一次 run 成功产出的文件路径（如 LAS 或 H5 列表）。
QStringList WorkflowEngine::outputsOf(const QString& id) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_records.constFind(id);
    return it == m_records.constEnd() ? QStringList{} : it->outputs;
}

/**
 * @brief 解析「单个上游节点」可提供给下游的文件路径。
 *
 * 优先级：
 *   1. 若上游已 run 过：用 NodeRecord::outputs（来自 NodeRunResult::outputFiles）
 *   2. 若是数据输入且尚未 run：读 params["input_files"]（属性面板选的 LAS）
 *
 * scheduleNode 会把结果写入下游 IProcessNode::m_inputs[ toPort ]。
 */
QStringList WorkflowEngine::resolveUpstreamFiles(const NodeRecord& rec) const
{
    QStringList raw;
    if (!rec.outputs.isEmpty())
        raw = rec.outputs;
    else if (rec.info.typeId == QLatin1String("input.data_input"))
        return pathsFromInputFilesParam(rec.info.params.value(QStringLiteral("input_files")));
    else
        return {};

    QStringList paths;
    for (const QString& p : raw)
    {
        const QString path = QDir::toNativeSeparators(p.trimmed());
        if (!path.isEmpty())
            paths.append(path);
    }
    return paths;
}

/**
 * @brief 查询「连到 toNodeId 某输入端口」的所有上游文件（去重合并）。
 * @param toPort 为空则合并所有入边；格式转换传 "in"
 *
 * 供属性面板只读展示「上游输入文件」，与 scheduleNode 注入逻辑一致。
 */
QStringList WorkflowEngine::upstreamInputFiles(const QString& toNodeId,
                                               const QString& toPort) const
{
    QMutexLocker lock(&m_mutex);
    QStringList merged;
    QSet<QString> seen;
    for (const auto& e : m_edges)
    {
        if (e.toNode != toNodeId)
            continue;
        if (!toPort.isEmpty() && e.toPort != toPort)
            continue;
        auto up = m_records.constFind(e.fromNode);
        if (up == m_records.constEnd())
            continue;
        for (const QString& path : resolveUpstreamFiles(*up))
        {
            if (seen.contains(path))
                continue;
            seen.insert(path);
            merged.append(path);
        }
    }
    return merged;
}

// -----------------------------------------------------------------------------
// 调度：单节点 / 整图
// -----------------------------------------------------------------------------

void WorkflowEngine::notifyNodeAborted(const QString& id, const QString& message)
{
    const NodeRunResult result{NodeStatus::Failed, message, {}};
    emit nodeStatusChanged(id, NodeStatus::Failed);
    emit nodeFinished(id, result);
    processing::LogBus::instance().post(processing::LogLevel::Error, id, message);
}

/**
 * @brief 收集某节点的所有上游祖先节点 ID（不含自身）。
 * 当前主要用于扩展；拓扑分层用 topoLayers。
 */
QSet<QString> WorkflowEngine::ancestors(const QString& id) const
{
    QSet<QString> out;
    QList<QString> q{id};
    while (!q.isEmpty())
    {
        auto cur = q.takeFirst();
        for (const auto& e : m_edges)
        {
            if (e.toNode == cur && !out.contains(e.fromNode))
            {
                out.insert(e.fromNode);
                q.append(e.fromNode);
            }
        }
    }
    return out;
}

/**
 * @brief 在后台线程执行一个节点的 run()。
 *
 * 主线程步骤：
 *   1. 标记 Running，发 nodeStatusChanged
 *   2. 遍历入边，resolveUpstreamFiles + setInputFiles → 写入下游 m_inputs
 *   3. QtConcurrent::run 在线程池里调用 node->run()
 *
 * 工作线程结束后（持锁写回）：
 *   status、outputs，再 emit nodeFinished（runSingle / runAll 都靠它收尾）
 *
 * 任意失败/节点消失路径必须 notifyNodeAborted 或 nodeFinished，否则 m_running 无法释放。
 */
void WorkflowEngine::scheduleNode(const QString& id)
{
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_records.find(id);
        if (it == m_records.end())
        {
            lock.unlock();
            notifyNodeAborted(id, QStringLiteral("节点不存在或已删除"));
            return;
        }
        if (!it->node)
        {
            lock.unlock();
            notifyNodeAborted(id, QStringLiteral("节点实例无效"));
            return;
        }
        it->status = NodeStatus::Running;
    }
    emit nodeStatusChanged(id, NodeStatus::Running);

    // 运行前把上游文件路径按「目标端口」灌进本节点 m_inputs（持锁内用 id 查找，避免悬空指针）
    {
        QMutexLocker lock(&m_mutex);
        auto it = m_records.find(id);
        if (it == m_records.end())
        {
            lock.unlock();
            notifyNodeAborted(id, QStringLiteral("节点不存在或已删除"));
            return;
        }
        if (!it->node)
        {
            lock.unlock();
            notifyNodeAborted(id, QStringLiteral("节点实例无效"));
            return;
        }
        for (const auto& e : m_edges)
        {
            if (e.toNode != id)
                continue;
            auto up = m_records.constFind(e.fromNode);
            if (up != m_records.constEnd())
                it->node->setInputFiles(e.toPort, resolveUpstreamFiles(*up));
        }
    }

    auto self = this;
    QtConcurrent::run(&m_pool, [self, id]()
                      {
                          const auto abortMsg = QStringLiteral("节点已移除，任务中止");
                          ProcessNodePtr node;
                          {
                              QMutexLocker lock(&self->m_mutex);
                              auto it = self->m_records.find(id);
                              if (it == self->m_records.end())
                              {
                                  lock.unlock();
                                  self->notifyNodeAborted(id, abortMsg);
                                  return;
                              }
                              node = it->node;
                              if (!node)
                              {
                                  lock.unlock();
                                  self->notifyNodeAborted(id, QStringLiteral("节点实例无效"));
                                  return;
                              }
                          }
                          auto result = node->run([self, id](int p, const QString& log)
                                                  {
                                                      emit self->nodeProgress(id, p, log);
                                                      processing::LogBus::instance().post(
                                                          processing::LogLevel::Info,
                                                          id,
                                                          p >= 0 ? QStringLiteral("[%1%] %2").arg(p).arg(log)
                                                                 : log);
                                                  });
                          {
                              QMutexLocker lock(&self->m_mutex);
                              auto it = self->m_records.find(id);
                              if (it == self->m_records.end())
                              {
                                  lock.unlock();
                                  self->notifyNodeAborted(id, abortMsg);
                                  return;
                              }
                              it->status = result.status;
                              it->outputs = result.outputFiles;
                          }
                          emit self->nodeStatusChanged(id, result.status);
                          emit self->nodeFinished(id, result);
                          processing::LogBus::instance().post(
                              result.status == NodeStatus::Failed ? processing::LogLevel::Error
                                                                   : processing::LogLevel::Info,
                              id,
                              QStringLiteral("finished: %1 %2")
                                  .arg(toString(result.status), result.message));
                      });
}

/**
 * @brief 只运行画布选中的一个节点（PDF 3.2.1）。
 *
 * 不自动先跑上游；上游路径靠 resolveUpstreamFiles（数据输入可读 params）。
 * m_running 原子锁防止重入；本节点 nodeFinished 后 finishRun。
 */
void WorkflowEngine::runSingle(const QString& id)
{
    if (m_running.exchange(true))
        return;
    m_stopRequested.store(false);
    emit runStarted();

    QMetaObject::Connection* connHolder = new QMetaObject::Connection;
    *connHolder = connect(this, &IWorkflowEngine::nodeFinished, this,
                          [this, id, connHolder](const QString& finishedId, const NodeRunResult& r)
                          {
                              if (finishedId != id)
                                  return;
                              QObject::disconnect(*connHolder);
                              delete connHolder;
                              finishRun(r.status == NodeStatus::Succeeded);
                          });
    scheduleNode(id);
}

/**
 * @brief 按拓扑层顺序运行整张图（PDF 3.2.2）。
 *
 * 同层节点并行 scheduleNode；一层全部 nodeFinished 后再 scheduleLayer(下一层)。
 */
void WorkflowEngine::runAll()
{
    if (m_running.exchange(true))
        return;
    m_stopRequested.store(false);
    emit runStarted();

    m_runLayers = topoLayers();
    m_runAllSucceeded = true;
    m_runCurrentLayer = -1;

    if (m_runLayers.isEmpty())
    {
        finishRun(true);
        return;
    }
    scheduleLayer(0);
}

/**
 * @brief 停止运行：通知各节点 requestCancel，并结束本次 run 会话。
 */
void WorkflowEngine::stop()
{
    m_stopRequested.store(true);
    {
        QMutexLocker lock(&m_mutex);
        for (auto& r : m_records)
        {
            if (r.node)
                r.node->requestCancel();
        }
    }
    if (m_running.load())
    {
        emit runStopped();
        finishRun(false);
    }
}

/// 释放 m_running，清空 runAll 临时状态，通知 UI runFinished。
void WorkflowEngine::finishRun(bool allSucceeded)
{
    if (!m_running.exchange(false))
        return;
    m_runLayers.clear();
    m_runPending.clear();
    m_runCurrentLayer = -1;
    emit runFinished(allSucceeded);
}

/**
 * @brief 拓扑分层，供 runAll 按层调度。
 *
 * 每一层：所有「直接上游都已出现在更早层」的节点。
 * 第 0 层 ≈ 无入边的节点（如数据输入）。
 * 若 validateEdge 正常，不会出现 cur 为空却还有未 done 节点（成环）。
 */
QList<QStringList> WorkflowEngine::topoLayers() const
{
    QMutexLocker lock(&m_mutex);
    QHash<QString, int> indeg;
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); ++it)
        indeg.insert(it.key(), 0);
    for (const auto& e : m_edges)
    {
        if (indeg.contains(e.toNode))
            indeg[e.toNode] += 1;
    }

    QList<QStringList> layers;
    QSet<QString> done;
    while (done.size() < indeg.size())
    {
        QStringList cur;
        for (auto it = indeg.constBegin(); it != indeg.constEnd(); ++it)
        {
            if (done.contains(it.key()))
                continue;
            bool ready = true;
            for (const auto& e : m_edges)
            {
                if (e.toNode == it.key() && !done.contains(e.fromNode))
                {
                    ready = false;
                    break;
                }
            }
            if (ready)
                cur.append(it.key());
        }
        if (cur.isEmpty())
            break;
        std::sort(cur.begin(), cur.end());
        layers.append(cur);
        for (const auto& nid : cur)
            done.insert(nid);
    }
    return layers;
}

/**
 * @brief 调度拓扑第 layerIndex 层：层内多节点并发，全部完成后进下一层。
 */
void WorkflowEngine::scheduleLayer(int layerIndex)
{
    m_runCurrentLayer = layerIndex;
    if (layerIndex < 0 || layerIndex >= m_runLayers.size())
    {
        finishRun(m_runAllSucceeded);
        return;
    }
    if (m_stopRequested.load())
    {
        finishRun(false);
        return;
    }

    const QStringList layer = m_runLayers.at(layerIndex);
    m_runPending = QSet<QString>(layer.begin(), layer.end());

    QMetaObject::Connection* connHolder = new QMetaObject::Connection;
    *connHolder = connect(this, &IWorkflowEngine::nodeFinished, this,
                          [this, layerIndex, connHolder](const QString& nid, const NodeRunResult& r)
                          {
                              Q_UNUSED(r);
                              if (!m_runPending.contains(nid))
                                  return;
                              m_runPending.remove(nid);
                              if (r.status != NodeStatus::Succeeded)
                                  m_runAllSucceeded = false;
                              if (m_runPending.isEmpty())
                              {
                                  QObject::disconnect(*connHolder);
                                  delete connHolder;
                                  scheduleLayer(layerIndex + 1);
                              }
                          });

    for (const auto& nid : layer)
        scheduleNode(nid);
}

// -----------------------------------------------------------------------------
// 序列化（预设 JSON / 工程保存）
// -----------------------------------------------------------------------------

/**
 * @brief 导出工作流为 JSON 字节。
 * @param includeData true 时带上 inputs/outputs 路径（工程）；false 仅拓扑+参数（模板）
 */
QByteArray WorkflowEngine::serialize(bool includeData) const
{
    QMutexLocker lock(&m_mutex);
    QJsonObject root;
    QJsonArray ns;
    for (const auto& r : m_records)
    {
        QJsonObject n;
        n["id"] = r.info.instanceId;
        n["type"] = r.info.typeId;
        n["name"] = r.info.displayName;
        n["x"] = r.info.position.x();
        n["y"] = r.info.position.y();
        if (r.info.size.isValid())
        {
            n["w"] = r.info.size.width();
            n["h"] = r.info.size.height();
        }
        n["params"] = QJsonObject::fromVariantMap(r.info.params);
        if (includeData)
        {
            QJsonArray inputs;
            for (const auto& f : r.info.inputFiles)
                inputs.append(f);
            n["inputs"] = inputs;
            QJsonArray outs;
            for (const auto& f : r.outputs)
                outs.append(f);
            n["outputs"] = outs;
        }
        ns.append(n);
    }
    QJsonArray es;
    for (const auto& e : m_edges)
    {
        QJsonObject je;
        je["from"] = e.fromNode;
        je["fromPort"] = e.fromPort;
        je["to"] = e.toNode;
        je["toPort"] = e.toPort;
        es.append(je);
    }
    root["nodes"] = ns;
    root["edges"] = es;
    root["withData"] = includeData;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * @brief 从 JSON 恢复工作流；会先 clear() 再逐节点/边重建并 emit 信号给 UI。
 */
bool WorkflowEngine::deserialize(const QByteArray& data)
{
    auto doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return false;
    clear();

    const auto root = doc.object();
    const auto withData = root.value("withData").toBool();

    for (const auto& v : root.value("nodes").toArray())
    {
        const auto n = v.toObject();
        const auto type = n.value("type").toString();
        if (!m_factory || !m_factory->hasType(type))
            continue;
        auto node = m_factory->create(type);
        NodeRecord rec;
        rec.info.instanceId = n.value("id").toString();
        rec.info.typeId = type;
        rec.info.displayName = n.value("name").toString();
        rec.info.position = QPointF(n.value("x").toDouble(), n.value("y").toDouble());
        if (n.contains("w") && n.contains("h"))
            rec.info.size = QSizeF(n.value("w").toDouble(), n.value("h").toDouble());
        rec.info.params = production::mergeWithClientDefaults(
            type, n.value("params").toObject().toVariantMap());
        if (withData)
        {
            for (const auto& f : n.value("inputs").toArray())
                rec.info.inputFiles.append(f.toString());
            for (const auto& f : n.value("outputs").toArray())
                rec.outputs.append(f.toString());
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

    for (const auto& v : root.value("edges").toArray())
    {
        const auto je = v.toObject();
        const EdgeInstance e{
            je.value("from").toString(),
            je.value("fromPort").toString(),
            je.value("to").toString(),
            je.value("toPort").toString()};

        QString reason;
        {
            QMutexLocker lock(&m_mutex);
            if (!validateEdge(e, &reason))
            {
                qWarning().noquote()
                    << QStringLiteral("[WorkflowEngine] deserialize 跳过非法边 %1:%2 -> %3:%4 — %5")
                           .arg(e.fromNode, e.fromPort, e.toNode, e.toPort, reason);
                continue;
            }
            m_edges.append(e);
        }
        emit edgeAdded(e);
    }
    return true;
}

/// 清空所有节点与边（先拷贝再 emit，避免遍历时修改容器）。
void WorkflowEngine::clear()
{
    QList<QString> ids;
    QList<EdgeInstance> es;
    {
        QMutexLocker lock(&m_mutex);
        ids = m_records.keys();
        es = m_edges;
        m_records.clear();
        m_edges.clear();
    }
    for (const auto& e : es)
        emit edgeRemoved(e);
    for (const auto& id : ids)
        emit nodeRemoved(id);
}

} // namespace processing
