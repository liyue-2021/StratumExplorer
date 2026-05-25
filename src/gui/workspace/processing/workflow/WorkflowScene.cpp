#include "WorkflowScene.h"
#include "NodeItem.h"
#include "EdgeItem.h"

#include "service/processing/IWorkflowEngine.h"
#include "service/processing/INodeFactory.h"
#include "processing/NodeDefinition.h"
#include "NodeCompatibility.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsLineItem>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMimeData>
#include <QPen>
#include <QSet>

using namespace processing;
using namespace processing::gui;

WorkflowScene::WorkflowScene(IWorkflowEngine *engine, INodeFactory *factory, QObject *parent)
    : QGraphicsScene(parent), m_engine(engine), m_factory(factory)
{
    setSceneRect(-2000, -2000, 4000, 4000);

    connect(m_engine, &IWorkflowEngine::nodeAdded, this, &WorkflowScene::onNodeAdded);
    connect(m_engine, &IWorkflowEngine::nodeRemoved, this, &WorkflowScene::onNodeRemoved);
    connect(m_engine, &IWorkflowEngine::nodeRenamed, this, &WorkflowScene::onNodeRenamed);
    connect(m_engine, &IWorkflowEngine::nodeStatusChanged, this, &WorkflowScene::onNodeStatusChanged);
    connect(m_engine, &IWorkflowEngine::edgeAdded, this, &WorkflowScene::onEdgeAdded);
    connect(m_engine, &IWorkflowEngine::edgeRemoved, this, &WorkflowScene::onEdgeRemoved);
}

QString WorkflowScene::selectedNodeId() const
{
    const auto items = selectedItems();
    for (auto *it : items)
        if (auto *n = dynamic_cast<NodeItem *>(it))
            return n->instanceId();
    return {};
}

void WorkflowScene::requestRunNode(const QString &id)
{
    if (m_engine && !id.isEmpty())
        m_engine->runSingle(id);
}
void WorkflowScene::requestStopAll()
{
    if (m_engine)
        m_engine->stop();
}

NodeItem *WorkflowScene::nodeItemAt(const QPointF &scenePos) const
{
    const auto its = items(scenePos);
    for (auto *it : its)
        if (auto *n = dynamic_cast<NodeItem *>(it))
            return n;
    return nullptr;
}

const NodeMeta *WorkflowScene::metaOf(const QString &typeId) const
{
    if (!m_factory)
        return nullptr;
    static thread_local NodeMeta cache;
    for (const auto &m : m_factory->listAll())
        if (m.typeId == typeId)
        {
            cache = m;
            return &cache;
        }
    return nullptr;
}

QString WorkflowScene::pickFirstFreeOutputPort(const QString &nodeId,
                                               const QString &typeId) const
{
    // 查询该节点类型的输出端口定义
    const NodeMeta *meta = metaOf(typeId);
    if (!meta || meta->outputs.isEmpty() || !m_engine)
        return {};

    // 输出端口允许 fan-out（一对多），所以直接返回第一个输出端口
    // 将来可在此处添加限制单端口扇出数量的逻辑
    return meta->outputs.first().key;
}

QString WorkflowScene::pickFirstFreeInputPort(const QString &nodeId,
                                              const QString &typeId,
                                              const QString &fromTypeId,
                                              DataFormat fromFormat) const
{
    // 查询该节点类型的输入端口定义
    const NodeMeta *meta = metaOf(typeId);
    const NodeMeta *fromMeta = metaOf(fromTypeId);
    if (!meta || meta->inputs.isEmpty() || !m_engine)
        return {};

    // 收集该节点上已被占用的目标端口（每个输入端口只能连一条边）
    QSet<QString> used;
    for (const auto &e : m_engine->edges())
        if (e.toNode == nodeId)
            used.insert(e.toPort);

    // 优先：未占用 + HDF5 格式兼容 + 算法语义兼容（与 WorkflowEngine::validateEdge 一致）
    for (const auto &p : meta->inputs)
    {
        if (used.contains(p.key) || !isCompatible(fromFormat, p.format))
            continue;
        if (fromMeta && !isAlgorithmEdgeCompatible(*fromMeta, *meta))
            continue;
        return p.key;
    }
    return {};
}

// ----------------------------------------------------- 引擎信号 → 视图

void WorkflowScene::onNodeAdded(const NodeInstance &n)
{
    // 从节点工厂里查询该节点类型的元数据（分组、节点类型、端口定义等）
    NodeGroup g = NodeGroup::Preprocess;
    NodeKind k = NodeKind::InOut;
    if (m_factory)
    {
        for (const auto &m : m_factory->listAll())
            if (m.typeId == n.typeId)
            {
                g = m.group;
                k = m.kind;
                break;
            }
    }
    // 创建新的节点视图对象并加入画布
    auto *item = new NodeItem(n, g, k);
    item->setPos(n.position);
    // 如果节点有历史尺寸数据则还原，否则使用默认尺寸
    if (n.size.isValid() && n.size.width() > 0 && n.size.height() > 0)
    {
        item->setSize(n.size.width(), n.size.height());
    }
    addItem(item);
    m_nodeItems.insert(n.instanceId, item);
}

void WorkflowScene::onNodeRemoved(const QString &id)
{
    // 从场景中移除该节点对应的视图，同时清理关联的所有边
    if (auto *item = m_nodeItems.take(id))
    {
        if (m_pendingFrom == item)
        {
            m_pendingFrom = nullptr;
            if (m_previewLine)
            {
                removeItem(m_previewLine);
                delete m_previewLine;
                m_previewLine = nullptr;
            }
        }
        for (auto it = m_edgeItems.begin(); it != m_edgeItems.end();)
        {
            if ((*it)->touches(item))
            {
                removeItem(*it);
                delete *it;
                it = m_edgeItems.erase(it);
            }
            else
                ++it;
        }
        removeItem(item);
        delete item;
    }
}

void WorkflowScene::clearAlignmentGuides()
{
    if (m_alignLineV)
    {
        removeItem(m_alignLineV);
        delete m_alignLineV;
        m_alignLineV = nullptr;
    }
    if (m_alignLineH)
    {
        removeItem(m_alignLineH);
        delete m_alignLineH;
        m_alignLineH = nullptr;
    }
    m_hasAlignmentX = false;
    m_hasAlignmentY = false;
    m_alignmentSnap = QPointF();
}

void WorkflowScene::updateAlignmentGuides(NodeItem *movingNode, const QPointF &proposedPos)
{
    if (!movingNode)
    {
        clearAlignmentGuides();
        return;
    }

    const QSizeF size = movingNode->currentSize();
    const QRectF movingRect(proposedPos, size);
    const qreal threshold = 12.0;
    qreal bestDistX = threshold + 1.0;
    qreal bestDistY = threshold + 1.0;
    qreal bestDX = 0.0;
    qreal bestDY = 0.0;
    qreal guideX = 0.0;
    qreal guideY = 0.0;
    const qreal movingLeft = movingRect.left();
    const qreal movingRight = movingRect.right();
    const qreal movingCenterX = movingRect.center().x();
    const qreal movingTop = movingRect.top();
    const qreal movingBottom = movingRect.bottom();
    const qreal movingCenterY = movingRect.center().y();

    for (auto *other : m_nodeItems.values())
    {
        if (!other || other == movingNode)
            continue;

        const QRectF otherRect(other->pos(), other->currentSize());
        const qreal otherLeft = otherRect.left();
        const qreal otherRight = otherRect.right();
        const qreal otherCenterX = otherRect.center().x();
        const qreal otherTop = otherRect.top();
        const qreal otherBottom = otherRect.bottom();
        const qreal otherCenterY = otherRect.center().y();

        auto checkX = [&](qreal targetX, qreal sourceX)
        {
            const qreal dx = targetX - sourceX;
            const qreal adx = qAbs(dx);
            if (adx < bestDistX)
            {
                bestDistX = adx;
                bestDX = dx;
                guideX = targetX;
            }
        };
        auto checkY = [&](qreal targetY, qreal sourceY)
        {
            const qreal dy = targetY - sourceY;
            const qreal ady = qAbs(dy);
            if (ady < bestDistY)
            {
                bestDistY = ady;
                bestDY = dy;
                guideY = targetY;
            }
        };

        checkX(otherLeft, movingLeft);
        checkX(otherLeft, movingRight);
        checkX(otherRight, movingLeft);
        checkX(otherRight, movingRight);
        checkX(otherCenterX, movingCenterX);

        checkY(otherTop, movingTop);
        checkY(otherTop, movingBottom);
        checkY(otherBottom, movingTop);
        checkY(otherBottom, movingBottom);
        checkY(otherCenterY, movingCenterY);
    }

    const bool snapX = bestDistX <= threshold;
    const bool snapY = bestDistY <= threshold;
    m_hasAlignmentX = snapX;
    m_hasAlignmentY = snapY;
    m_alignmentSnap = QPointF(snapX ? bestDX : 0.0, snapY ? bestDY : 0.0);

    QPen pen(QColor("#64B5F6"), 1, Qt::DashLine);
    pen.setCosmetic(true);

    if (snapX)
    {
        if (!m_alignLineV)
        {
            m_alignLineV = new QGraphicsLineItem();
            m_alignLineV->setZValue(100);
            addItem(m_alignLineV);
        }
        m_alignLineV->setPen(pen);
        m_alignLineV->setLine(QLineF(QPointF(guideX, sceneRect().top()),
                                     QPointF(guideX, sceneRect().bottom())));
    }
    else if (m_alignLineV)
    {
        removeItem(m_alignLineV);
        delete m_alignLineV;
        m_alignLineV = nullptr;
    }

    if (snapY)
    {
        if (!m_alignLineH)
        {
            m_alignLineH = new QGraphicsLineItem();
            m_alignLineH->setZValue(100);
            addItem(m_alignLineH);
        }
        m_alignLineH->setPen(pen);
        m_alignLineH->setLine(QLineF(QPointF(sceneRect().left(), guideY),
                                     QPointF(sceneRect().right(), guideY)));
    }
    else if (m_alignLineH)
    {
        removeItem(m_alignLineH);
        delete m_alignLineH;
        m_alignLineH = nullptr;
    }
}

void WorkflowScene::applyAlignmentSnap(NodeItem *movingNode)
{
    if (!movingNode || (!m_hasAlignmentX && !m_hasAlignmentY))
        return;

    const QPointF currentPos = movingNode->pos();
    const QPointF targetPos(currentPos.x() + (m_hasAlignmentX ? m_alignmentSnap.x() : 0.0),
                            currentPos.y() + (m_hasAlignmentY ? m_alignmentSnap.y() : 0.0));
    if (!qFuzzyCompare(currentPos.x(), targetPos.x()) || !qFuzzyCompare(currentPos.y(), targetPos.y()))
        movingNode->setPos(targetPos);
}

void WorkflowScene::onNodeRenamed(const QString &id, const QString &newName)
{
    if (auto *item = m_nodeItems.value(id, nullptr))
    {
        item->setTitle(newName);
    }
}

void WorkflowScene::onNodeStatusChanged(const QString &id, NodeStatus s)
{
    if (auto *item = m_nodeItems.value(id, nullptr))
        item->setStatus(s);
}

void WorkflowScene::onEdgeAdded(const EdgeInstance &e)
{
    // 查找源/目标节点的视图对象
    auto *from = m_nodeItems.value(e.fromNode, nullptr);
    auto *to = m_nodeItems.value(e.toNode, nullptr);
    if (!from || !to)
        return;
    // 创建视觉边对象并加入画布，边会自动跟随节点移动
    auto *edge = new EdgeItem(from, e.fromPort, to, e.toPort);
    addItem(edge);
    m_edgeItems.append(edge);
}

void WorkflowScene::onEdgeRemoved(const EdgeInstance &e)
{
    for (auto it = m_edgeItems.begin(); it != m_edgeItems.end();)
    {
        if ((*it)->matches(e.fromNode, e.toNode, e.fromPort, e.toPort))
        {
            removeItem(*it);
            delete *it;
            it = m_edgeItems.erase(it);
        }
        else
            ++it;
    }
}

// ----------------------------------------------------- 拖拽连线

void WorkflowScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (auto *n = nodeItemAt(e->scenePos()))
        {
            const QPointF local = n->mapFromScene(e->scenePos());
            bool hit = false;
            (void)n->hitOutputSide(local, &hit);
            if (hit)
            {
                m_pendingFrom = n;
                m_previewLine = new QGraphicsLineItem(
                    QLineF(e->scenePos(), e->scenePos()));
                QPen pen(QColor("#1976D2"), 2, Qt::DashLine);
                m_previewLine->setPen(pen);
                m_previewLine->setZValue(10);
                addItem(m_previewLine);
                e->accept();
                return;
            }
        }
    }
    QGraphicsScene::mousePressEvent(e);
}

void WorkflowScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    // 双击节点正文（不在端口/按钮范围）→ 通知 Tab 弹出可视化窗口
    if (e->button() == Qt::LeftButton)
    {
        if (auto *n = nodeItemAt(e->scenePos()))
        {
            emit nodeDoubleClicked(n->instanceId());
            e->accept();
            return;
        }
    }
    QGraphicsScene::mouseDoubleClickEvent(e);
}

void WorkflowScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_previewLine && m_pendingFrom)
    {
        // 预览线起点用源节点离当前光标更近的输出锚（自动适应方向）
        const QPointF cur = e->scenePos();
        const QPointF aR = m_pendingFrom->anchorScene(NodeItem::Side::Right);
        const QPointF aB = m_pendingFrom->anchorScene(NodeItem::Side::Bottom);
        auto d2 = [&](const QPointF &a)
        {
            const qreal dx = a.x() - cur.x(), dy = a.y() - cur.y();
            return dx * dx + dy * dy;
        };
        const QPointF start = (d2(aR) <= d2(aB) || m_pendingFrom->kind() == NodeKind::PureInput)
                                  ? aR
                                  : aB;
        m_previewLine->setLine(QLineF(start, cur));
        e->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(e);
}

void WorkflowScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_previewLine && m_pendingFrom)
    {
        if (m_previewLine)
        {
            removeItem(m_previewLine);
            delete m_previewLine;
            m_previewLine = nullptr;
        }
        NodeItem *from = m_pendingFrom;
        m_pendingFrom = nullptr;

        if (auto *target = nodeItemAt(e->scenePos()))
        {
            if (target != from && m_engine)
            {
                const QPointF local = target->mapFromScene(e->scenePos());
                bool hit = false;
                (void)target->hitInputSide(local, &hit);
                // 也支持落到节点卡片内任意位置（视为命中输入端口，便于使用）
                if (!hit)
                    hit = true;

                if (hit)
                {
                    const QString fromPort = pickFirstFreeOutputPort(
                        from->instanceId(), from->typeId());

                    DataFormat outFmt = DataFormat::Unknown;
                    if (const NodeMeta *fm = metaOf(from->typeId()))
                    {
                        for (const auto &p : fm->outputs)
                            if (p.key == fromPort)
                            {
                                outFmt = p.format;
                                break;
                            }
                    }
                    QString toPort = pickFirstFreeInputPort(
                        target->instanceId(), target->typeId(), from->typeId(), outFmt);

                    // 目标节点所有 input 端口都被占用 → 弹菜单允许"替换某条已有连线"
                    if (!fromPort.isEmpty() && toPort.isEmpty())
                    {
                        const NodeMeta *tm = metaOf(target->typeId());
                        if (tm && !tm->inputs.isEmpty())
                        {
                            // 收集占用了目标 input 端口的现有边
                            QList<EdgeInstance> occupying;
                            for (const auto &ed : m_engine->edges())
                                if (ed.toNode == target->instanceId())
                                    occupying.append(ed);

                            if (!occupying.isEmpty())
                            {
                                QMenu replaceMenu;
                                replaceMenu.addAction(tr("替换已有连线："))->setEnabled(false);
                                replaceMenu.addSeparator();
                                QHash<QAction *, EdgeInstance> map;
                                for (const auto &ed : occupying)
                                {
                                    // 用源节点显示名 + 端口名做标签
                                    QString srcName = ed.fromNode;
                                    for (const auto &nn : m_engine->nodes())
                                        if (nn.instanceId == ed.fromNode)
                                        {
                                            srcName = nn.displayName;
                                            break;
                                        }
                                    auto *a = replaceMenu.addAction(
                                        tr("用本连线替换  [%1] → 端口 %2")
                                            .arg(srcName, ed.toPort));
                                    map.insert(a, ed);
                                }
                                replaceMenu.addSeparator();
                                QAction *aCancel = replaceMenu.addAction(tr("取消"));
                                QAction *sel = replaceMenu.exec(e->screenPos());
                                if (sel && sel != aCancel && map.contains(sel))
                                {
                                    const EdgeInstance victim = map.value(sel);
                                    toPort = victim.toPort;
                                    m_engine->removeEdge(victim);
                                }
                            }
                        }
                    }

                    if (fromPort.isEmpty() || toPort.isEmpty())
                    {
                        QString reason;
                        const NodeMeta *tm = metaOf(target->typeId());
                        const NodeMeta *fm = metaOf(from->typeId());
                        if (fromPort.isEmpty())
                        {
                            reason = tr("源节点 %1 没有可用的输出端口").arg(fm ? fm->displayName : from->typeId());
                        }
                        else if (tm && tm->inputs.isEmpty())
                        {
                            reason = tr("目标节点 %1 没有输入端口").arg(tm->displayName);
                        }
                        else if (fromMeta && tm
                                 && !isAlgorithmEdgeCompatible(*fromMeta, *tm, &reason))
                        {
                            /* reason 已由兼容性模块填写 */
                        }
                        else
                        {
                            reason = tr("目标节点没有可连接的输入端口（可能已被占用或算法不兼容）");
                        }
                        EdgeInstance dummy{from->instanceId(), QString(),
                                           target->instanceId(), QString()};
                        emit m_engine->edgeRejected(dummy, reason);
                    }
                    else
                    {
                        EdgeInstance edge{from->instanceId(), fromPort,
                                          target->instanceId(), toPort};
                        m_engine->addEdge(edge);
                    }
                }
            }
        }
        e->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(e);
}

// ----------------------------------------------------- 节点右键菜单

void WorkflowScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *e)
{
    auto *n = nodeItemAt(e->scenePos());
    if (!n || !m_engine)
    {
        QGraphicsScene::contextMenuEvent(e);
        return;
    }

    const QString id = n->instanceId();
    const QString typeId = n->typeId();

    QMenu menu;
    QAction *aRun = menu.addAction(tr("运行此节点"));
    QAction *aRename = menu.addAction(tr("重命名..."));
    QAction *aReset = menu.addAction(tr("重置参数"));
    menu.addSeparator();
    QAction *aClearEdges = menu.addAction(tr("清除此节点连线"));
    QAction *aDelete = menu.addAction(tr("删除节点"));

    QAction *sel = menu.exec(e->screenPos());
    if (!sel)
    {
        e->accept();
        return;
    }

    if (sel == aRun)
    {
        m_engine->runSingle(id);
    }
    else if (sel == aDelete)
    {
        m_engine->removeNode(id);
    }
    else if (sel == aClearEdges)
    {
        const auto all = m_engine->edges();
        for (const auto &edge : all)
            if (edge.fromNode == id || edge.toNode == id)
                m_engine->removeEdge(edge);
    }
    else if (sel == aRename)
    {
        QString cur;
        for (const auto &nn : m_engine->nodes())
            if (nn.instanceId == id)
            {
                cur = nn.displayName;
                break;
            }
        bool ok = false;
        const QString name = QInputDialog::getText(
            nullptr, tr("重命名节点"),
            tr("新名称："), QLineEdit::Normal, cur, &ok);
        if (ok && !name.trimmed().isEmpty())
            m_engine->renameNode(id, name.trimmed());
    }
    else if (sel == aReset)
    {
        if (const NodeMeta *m = metaOf(typeId))
            m_engine->setNodeParams(id, m->defaultParams);
    }
    e->accept();
}

// ----------------------------------------------------- 节点库拖入画布

void WorkflowScene::dragEnterEvent(QGraphicsSceneDragDropEvent *e)
{
    if (e->mimeData() && e->mimeData()->hasFormat(kNodeTypeMime))
    {
        e->setDropAction(Qt::CopyAction);
        e->accept();
        return;
    }
    QGraphicsScene::dragEnterEvent(e);
}
void WorkflowScene::dragMoveEvent(QGraphicsSceneDragDropEvent *e)
{
    if (e->mimeData() && e->mimeData()->hasFormat(kNodeTypeMime))
    {
        e->setDropAction(Qt::CopyAction);
        e->accept();
        return;
    }
    QGraphicsScene::dragMoveEvent(e);
}
void WorkflowScene::dropEvent(QGraphicsSceneDragDropEvent *e)
{
    if (e->mimeData() && e->mimeData()->hasFormat(kNodeTypeMime) && m_engine)
    {
        const QString typeId = QString::fromUtf8(e->mimeData()->data(kNodeTypeMime));
        if (!typeId.isEmpty())
        {
            const QPointF pos = e->scenePos() - QPointF(80, 30);
            m_engine->addNode(typeId, pos);
        }
        e->setDropAction(Qt::CopyAction);
        e->accept();
        return;
    }
    QGraphicsScene::dropEvent(e);
}
