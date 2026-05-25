#include "NodeItem.h"
#include "EdgeItem.h"
#include "WorkflowScene.h"

#include <QPainter>
#include <QFontMetricsF>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

using namespace processing;
using namespace processing::gui;

namespace
{
    constexpr qreal kPad = 10.0;
    constexpr qreal kPort = 7.0;
    constexpr qreal kPortHit = 9.0;
    constexpr qreal kMinW = 160.0;
    constexpr qreal kMaxW = 380.0;
    constexpr qreal kMinH = 78.0;
    constexpr qreal kMaxH = 320.0;
    constexpr qreal kBtnSize = 18.0;
    constexpr qreal kGrip = 12.0; // 右下 resize 拖柄边长
} // namespace

NodeItem::NodeItem(const NodeInstance &info, NodeGroup group, NodeKind kind)
    : QGraphicsObject(),
      m_id(info.instanceId), m_title(info.displayName), m_typeId(info.typeId),
      m_group(group), m_kind(kind)
{
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setToolTip(QStringLiteral("%1\n类型: %2").arg(m_title, m_typeId));
    recomputeSize();
}

void NodeItem::setStatus(NodeStatus s)
{
    if (m_status == s)
        return;
    m_status = s;
    update();
}

void NodeItem::setTestSeqLabel(const QString &label)
{
    m_testSeqLabel = label.trimmed();
    setToolTip(QStringLiteral("%1\n序号: %2\n类型: %3")
                   .arg(m_title,
                        m_testSeqLabel.isEmpty() ? QStringLiteral("—") : m_testSeqLabel,
                        m_typeId));
    prepareGeometryChange();
    recomputeSize();
    update();
}

void NodeItem::setTitle(const QString &title)
{
    m_title = title;
    setToolTip(QStringLiteral("%1\n序号: %2\n类型: %3")
                   .arg(m_title,
                        m_testSeqLabel.isEmpty() ? QStringLiteral("—") : m_testSeqLabel,
                        m_typeId));
    prepareGeometryChange();
    recomputeSize();
    update();
    for (auto *e : std::as_const(m_edges))
        if (e)
            e->onEndpointGeometryChanged();
}

void NodeItem::setSize(qreal w, qreal h)
{
    // 先 recomputeSize() 确保 m_minWidth/m_minHeight 是最新的
    recomputeSize();
    const qreal cw = qBound(m_minWidth, w, qreal(380.0));
    const qreal ch = qBound(m_minHeight, h, qreal(320.0));
    if (qFuzzyCompare(cw, m_width) && qFuzzyCompare(ch, m_height))
        return;
    prepareGeometryChange();
    m_width = cw;
    m_height = ch;
    update();
    for (auto *e : std::as_const(m_edges))
        if (e)
            e->onEndpointGeometryChanged();
}

QRectF NodeItem::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height).adjusted(-4, -4, 4, 4);
}

bool NodeItem::hasInput(Side s) const
{
    // PureInput 节点：4 侧全为输入（方便在任何方向接入）
    // InOut 节点：仅左/上 为输入侧（保持“进从一侧 / 出从另一侧”的语义）
    // PureOutput 节点：无输入
    if (m_kind == NodeKind::PureOutput)
        return false;
    if (m_kind == NodeKind::PureInput)
        return true;
    return s == Side::Left || s == Side::Top;
}
bool NodeItem::hasOutput(Side s) const
{
    if (m_kind == NodeKind::PureInput)
        return false;
    if (m_kind == NodeKind::PureOutput)
        return true;
    return s == Side::Right || s == Side::Bottom;
}

QPointF NodeItem::portCenter(Side s) const
{
    switch (s)
    {
    case Side::Left:
        return QPointF(0, m_height / 2.0);
    case Side::Right:
        return QPointF(m_width, m_height / 2.0);
    case Side::Top:
        return QPointF(m_width / 2.0, 0);
    case Side::Bottom:
        return QPointF(m_width / 2.0, m_height);
    }
    return {};
}

QPointF NodeItem::anchorScene(Side s) const { return mapToScene(portCenter(s)); }

NodeItem::Side NodeItem::hitInputSide(const QPointF &local, bool *hit) const
{
    if (hit)
        *hit = false;
    for (Side s : {Side::Left, Side::Top, Side::Right, Side::Bottom})
    {
        if (!hasInput(s))
            continue;
        const QPointF c = portCenter(s);
        const qreal dx = local.x() - c.x(), dy = local.y() - c.y();
        if (dx * dx + dy * dy <= kPortHit * kPortHit)
        {
            if (hit)
                *hit = true;
            return s;
        }
    }
    return Side::Left;
}

NodeItem::Side NodeItem::hitOutputSide(const QPointF &local, bool *hit) const
{
    if (hit)
        *hit = false;
    for (Side s : {Side::Right, Side::Bottom, Side::Left, Side::Top})
    {
        if (!hasOutput(s))
            continue;
        const QPointF c = portCenter(s);
        const qreal dx = local.x() - c.x(), dy = local.y() - c.y();
        if (dx * dx + dy * dy <= kPortHit * kPortHit)
        {
            if (hit)
                *hit = true;
            return s;
        }
    }
    return Side::Right;
}

void NodeItem::attachEdge(EdgeItem *e)
{
    if (e)
        m_edges.insert(e);
}
void NodeItem::detachEdge(EdgeItem *e)
{
    if (e)
        m_edges.remove(e);
}

QColor NodeItem::groupColor() const
{
    switch (m_group)
    {
    case NodeGroup::DataInput:
        return QColor("#E3F2FD");
    case NodeGroup::Preprocess:
        return QColor("#FFF3E0");
    case NodeGroup::Interpret:
        return QColor("#F3E5F5");
    case NodeGroup::Display:
        return QColor("#E8F5E9");
    }
    return Qt::white;
}

QColor NodeItem::statusColor() const
{
    switch (m_status)
    {
    case NodeStatus::Idle:
        return QColor("#9E9E9E");
    case NodeStatus::Running:
        return QColor("#FFA000");
    case NodeStatus::Succeeded:
        return QColor("#43A047");
    case NodeStatus::Failed:
        return QColor("#E53935");
    case NodeStatus::Stopped:
        return QColor("#616161");
    case NodeStatus::Error:
        return QColor("#AD1457");
    }
    return Qt::black;
}

void NodeItem::recomputeSize()
{
    // 根据文本内容计算节点的下限尺寸
    QFont fTitle;
    fTitle.setBold(true);
    fTitle.setPointSize(10);
    QFont fSub;
    fSub.setPointSize(8);

    QFontMetricsF fmT(fTitle), fmS(fSub);
    qreal wTitle = fmT.horizontalAdvance(m_title) + kPad * 2 + kBtnSize * 2 + 8;
    qreal wSeq = m_testSeqLabel.isEmpty() ? 0.0 : fmS.horizontalAdvance(m_testSeqLabel) + 24.0;
    qreal wType = fmS.horizontalAdvance(m_typeId) + kPad * 2;
    qreal wStat = fmS.horizontalAdvance(toString(m_status)) + kPad * 2;

    // 声明下限宽度，树立一个拖布的基準线、并对上限有有效控制
    m_minWidth = qBound(kMinW, qMax(wTitle, qMax(wSeq, qMax(wType, wStat))), kMaxW);
    m_minHeight = kMinH;
    // 如果当前宽/高不足，扩转为下限；否则保留用户拖拽的尺寸
    if (m_width < m_minWidth)
        m_width = m_minWidth;
    if (m_width > kMaxW)
        m_width = kMaxW;
    if (m_height < m_minHeight)
        m_height = m_minHeight;
    if (m_height > kMaxH)
        m_height = kMaxH;
}

QRectF NodeItem::gripRect() const
{
    return QRectF(m_width - kGrip, m_height - kGrip, kGrip, kGrip);
}

void NodeItem::paint(QPainter *p, const QStyleOptionGraphicsItem *opt, QWidget *)
{
    const QRectF r(0, 0, m_width, m_height);
    p->setRenderHint(QPainter::Antialiasing);

    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 40));
    p->drawRoundedRect(r.adjusted(2, 3, 2, 3), 8, 8);

    QColor border = (opt->state & QStyle::State_Selected) ? QColor("#1976D2") : QColor("#90A4AE");
    p->setPen(QPen(border, (opt->state & QStyle::State_Selected) ? 2.5 : 1.2));
    p->setBrush(groupColor());
    p->drawRoundedRect(r, 8, 8);

    QColor band = groupColor().darker(115);
    p->setPen(Qt::NoPen);
    p->setBrush(band);
    p->save();
    QPainterPath clip;
    clip.addRoundedRect(r, 8, 8);
    p->setClipPath(clip);
    p->drawRect(QRectF(0, 0, m_width, 22));
    p->restore();

    QFont fTitle;
    fTitle.setBold(true);
    fTitle.setPointSize(10);
    p->setFont(fTitle);
    p->setPen(QColor("#212121"));
    QFontMetricsF fmT(fTitle);
    QString elidedTitle = fmT.elidedText(
        m_title, Qt::ElideRight, m_width - kPad * 2 - kBtnSize * 2 - 8);
    const qreal btnLeft = m_width - kPad - kBtnSize * 2 - 4;
    qreal seqRight = btnLeft - 4;
    if (!m_testSeqLabel.isEmpty())
    {
        QFont fSeq;
        fSeq.setBold(true);
        fSeq.setPointSize(8);
        p->setFont(fSeq);
        QFontMetricsF fmSeq(fSeq);
        const qreal seqW = fmSeq.horizontalAdvance(m_testSeqLabel) + 10.0;
        seqRight = btnLeft - seqW - 4.0;
        const QRectF seqRect(seqRight, 4, seqW, 16);
        p->setPen(QPen(QColor("#E65100"), 1));
        p->setBrush(QColor("#FFF3E0"));
        p->drawRoundedRect(seqRect, 4, 4);
        p->setPen(QColor("#E65100"));
        p->drawText(seqRect, Qt::AlignCenter, m_testSeqLabel);
        fTitle.setBold(true);
        fTitle.setPointSize(10);
        p->setFont(fTitle);
        p->setPen(QColor("#212121"));
    }

    p->drawText(QRectF(kPad, 2, qMax(40.0, seqRight - kPad), 20),
                Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

    QRectF btnRun(btnLeft, 3, kBtnSize, kBtnSize);
    QRectF btnStop(m_width - kPad - kBtnSize, 3, kBtnSize, kBtnSize);
    p->setBrush(Qt::white);
    p->setPen(QPen(QColor("#607D8B"), 0.8));
    p->drawEllipse(btnRun);
    p->drawEllipse(btnStop);
    QPolygonF play({QPointF(btnRun.center().x() - 3, btnRun.center().y() - 4),
                    QPointF(btnRun.center().x() - 3, btnRun.center().y() + 4),
                    QPointF(btnRun.center().x() + 4, btnRun.center().y())});
    p->setBrush(QColor("#43A047"));
    p->setPen(Qt::NoPen);
    p->drawPolygon(play);
    p->setBrush(QColor("#E53935"));
    p->drawRect(QRectF(btnStop.center().x() - 3.5, btnStop.center().y() - 3.5, 7, 7));

    QFont fSub;
    fSub.setPointSize(8);
    p->setFont(fSub);
    p->setPen(QColor("#546E7A"));
    QFontMetricsF fmS(fSub);
    p->drawText(QRectF(kPad, 28, m_width - kPad * 2, 16),
                Qt::AlignLeft | Qt::AlignVCenter,
                fmS.elidedText(m_typeId, Qt::ElideRight, m_width - kPad * 2));

    p->setPen(statusColor());
    p->drawText(QRectF(kPad, 50, m_width - kPad * 2, 16),
                Qt::AlignLeft | Qt::AlignVCenter,
                QStringLiteral("● ") + toString(m_status));

    auto drawPort = [&](Side s, bool isInput)
    {
        const QPointF c = portCenter(s);
        p->setPen(QPen(QColor("#37474F"), 1));
        p->setBrush(isInput ? QColor("#FFF59D") : QColor("#A5D6A7"));
        p->drawEllipse(c, kPort / 2, kPort / 2);
    };

    // 只在鼠标悬停时显示端点
    if (m_hovering)
    {
        for (Side s : {Side::Left, Side::Right, Side::Top, Side::Bottom})
        {
            if (hasInput(s))
                drawPort(s, true);
            if (hasOutput(s))
                drawPort(s, false);
        }
    }
    else
    {
        // 鼠标未悬停时显示淡色方向指示器（可选：用微弱的箭头提示）
        // 保持美观但提供方向线索
        QColor hintColor(200, 200, 200, 80);
        p->setPen(QPen(hintColor, 0.5));

        // 左上半部分：输入侧（淡黄色提示）
        if (m_kind == NodeKind::PureInput || m_kind == NodeKind::InOut)
        {
            p->setBrush(QColor(255, 245, 157, 20)); // 淡黄
            p->drawEllipse(QPointF(kPad, kPad), 3, 3);
            p->drawEllipse(QPointF(m_width / 2.0, 2), 3, 3);
        }

        // 右下半部分：输出侧（淡绿色提示）
        if (m_kind == NodeKind::PureOutput || m_kind == NodeKind::InOut)
        {
            p->setBrush(QColor(165, 214, 167, 20)); // 淡绿
            p->drawEllipse(QPointF(m_width - kPad, m_height - kPad), 3, 3);
            p->drawEllipse(QPointF(m_width / 2.0, m_height - 2), 3, 3);
        }
    }

    // 右下 resize 拖柄：三条斜线
    {
        const QRectF g = gripRect();
        p->setPen(QPen(QColor("#90A4AE"), 1));
        for (int i = 0; i < 3; ++i)
        {
            const qreal off = 2.0 + i * 2.5;
            p->drawLine(QPointF(g.right() - off, g.bottom() - 1),
                        QPointF(g.right() - 1, g.bottom() - off));
        }
    }
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    // 1. 检查是否点击了运行按钮
    QRectF btnRun(m_width - kPad - kBtnSize * 2 - 4, 3, kBtnSize, kBtnSize);
    QRectF btnStop(m_width - kPad - kBtnSize, 3, kBtnSize, kBtnSize);
    auto *ws = dynamic_cast<WorkflowScene *>(scene());
    if (btnRun.contains(e->pos()))
    {
        if (ws)
            ws->requestRunNode(m_id);
        e->accept();
        return;
    }
    // 2. 检查是否点击了停止按钮
    if (btnStop.contains(e->pos()))
    {
        if (ws)
            ws->requestStopAll();
        e->accept();
        return;
    }
    // 3. 检查是否点击了右下角的拖动括手
    if (gripRect().contains(e->pos()) && e->button() == Qt::LeftButton)
    {
        m_resizing = true;
        m_resizeStartX = e->scenePos().x();
        m_resizeStartY = e->scenePos().y();
        m_resizeStartW = m_width;
        m_resizeStartH = m_height;
        e->accept();
        return;
    }
    // 4. 传透给标准 QGraphicsObject 事件处理（支持选中、拖动常规）
    QGraphicsObject::mousePressEvent(e);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_resizing)
    {
        const qreal dx = e->scenePos().x() - m_resizeStartX;
        const qreal dy = e->scenePos().y() - m_resizeStartY;
        const qreal w = qBound(m_minWidth, m_resizeStartW + dx, kMaxW);
        const qreal h = qBound(m_minHeight, m_resizeStartH + dy, kMaxH);
        if (!qFuzzyCompare(w, m_width) || !qFuzzyCompare(h, m_height))
        {
            prepareGeometryChange();
            m_width = w;
            m_height = h;
            update();
            for (auto *ed : std::as_const(m_edges))
                if (ed)
                    ed->onEndpointGeometryChanged();
        }
        e->accept();
        return;
    }

    if (e->buttons() & Qt::LeftButton)
        m_dragging = true;

    QGraphicsObject::mouseMoveEvent(e);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    if (m_resizing)
    {
        m_resizing = false;
        if (auto *ws = dynamic_cast<WorkflowScene *>(scene()))
        {
            if (auto *eng = ws->engine())
                eng->setNodeSize(m_id, QSizeF(m_width, m_height));
        }
        e->accept();
        return;
    }

    QGraphicsObject::mouseReleaseEvent(e);

    if (m_dragging)
    {
        m_dragging = false;
        if (auto *ws = dynamic_cast<WorkflowScene *>(scene()))
        {
            ws->applyAlignmentSnap(this);
            ws->clearAlignmentGuides();
        }
    }
}

void NodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e)
{
    bool wasHovering = m_hovering;
    m_hovering = true;
    if (!wasHovering)
        update(); // 鼠标进入时，重绘以显示端点

    setCursor(gripRect().contains(e->pos()) ? Qt::SizeFDiagCursor : Qt::ArrowCursor);
    QGraphicsObject::hoverMoveEvent(e);
}

void NodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    if (m_hovering)
    {
        m_hovering = false;
        update(); // 鼠标离开时，重绘以隐藏端点
    }
    QGraphicsObject::hoverLeaveEvent(e);
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && m_dragging)
    {
        if (auto *ws = dynamic_cast<WorkflowScene *>(scene()))
            ws->updateAlignmentGuides(this, value.toPointF());
    }
    if (change == ItemPositionHasChanged || change == ItemTransformHasChanged || change == ItemSceneHasChanged)
    {
        for (auto *e : std::as_const(m_edges))
            if (e)
                e->onEndpointGeometryChanged();
    }
    if (change == ItemPositionHasChanged)
    {
        // 实时回写位置，保证序列化能拿到用户拖动后的坐标。
        // 这个路径在拖动中会频繁触发，引擎实现里不 emit 信号，开销很小。
        if (auto *ws = dynamic_cast<WorkflowScene *>(scene()))
        {
            if (auto *eng = ws->engine())
                eng->setNodePosition(m_id, value.toPointF());
        }
    }
    return QGraphicsObject::itemChange(change, value);
}
