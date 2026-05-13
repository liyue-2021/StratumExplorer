#include "EdgeItem.h"
#include "NodeItem.h"

#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QtMath>
#include <limits>

using namespace processing::gui;

namespace {
// 全局当前样式（曲线/折线）。
EdgeItem::Style g_style = EdgeItem::Style::Bezier;
constexpr qreal kStub = 18.0;   // 折线模式：每端伸出一截再拐弯
} // namespace

EdgeItem::Style EdgeItem::style()              { return g_style; }
void            EdgeItem::setStyle(Style s)    { g_style = s;    }

EdgeItem::EdgeItem(NodeItem* from, const QString& fromPort,
                   NodeItem* to,   const QString& toPort)
    : m_from(from), m_to(to), m_fromPort(fromPort), m_toPort(toPort) {
    setZValue(-1);
    if (m_from) m_from->attachEdge(this);
    if (m_to)   m_to->attachEdge(this);
}

EdgeItem::~EdgeItem() {
    // QPointer 会在节点已被删除时为空，安全。
    if (m_from) m_from->detachEdge(this);
    if (m_to)   m_to->detachEdge(this);
}

bool EdgeItem::matches(const QString& fromId, const QString& toId,
                       const QString& fromPort, const QString& toPort) const {
    if (!m_from || !m_to) return false;
    if (m_from->instanceId() != fromId) return false;
    if (m_to->instanceId()   != toId)   return false;
    if (!fromPort.isEmpty() && m_fromPort != fromPort) return false;
    if (!toPort.isEmpty()   && m_toPort   != toPort)   return false;
    return true;
}

bool EdgeItem::touches(NodeItem* node) const {
    return node && (m_from.data() == node || m_to.data() == node);
}

void EdgeItem::onEndpointGeometryChanged() {
    prepareGeometryChange();
    update();
}

// 在 (源所有 output 侧) × (目标所有 input 侧) 中挑"距离 + 朝向"综合最优的一对。
// PureInput 节点 4 侧均可入；PureOutput 节点 4 侧均可出；InOut 仅 right/bottom = out，left/top = in。
//
// 之前只比平方距离，导致两节点同高时会选成 LAS.right→带通.right（同侧角对角，
// 视觉上"绕一圈"），或者上方节点用 bottom 出却选了 right 出。这里改用：
//   cost = 距离 - 强奖励(出端口朝向指向对端) - 强奖励(入端口朝向迎向源)
// 朝向匹配度用单位向量点积衡量（1 = 完全朝向，-1 = 背离）。
void EdgeItem::pickEndpoints() const {
    if (!m_from || !m_to) {
        m_start = m_end = QPointF();
        return;
    }
    using NSide = NodeItem::Side;
    const NSide allSides[4] = { NSide::Right, NSide::Bottom, NSide::Left, NSide::Top };
    auto toEdgeSide = [](NSide n) -> Side {
        switch (n) {
            case NSide::Right:  return Right;
            case NSide::Bottom: return Bottom;
            case NSide::Left:   return Left;
            case NSide::Top:    return Top;
        }
        return Right;
    };
    auto sideDir = [](NSide s) -> QPointF {
        switch (s) {
            case NSide::Right:  return QPointF( 1,  0);
            case NSide::Left:   return QPointF(-1,  0);
            case NSide::Bottom: return QPointF( 0,  1);
            case NSide::Top:    return QPointF( 0, -1);
        }
        return {};
    };

    // 朝向奖励的权重：相当于"完全朝向 = 节省 ALIGN_BONUS 单位的距离"
    // 调大可让端口更"听话"，调小则更接近纯距离最近。
    constexpr qreal ALIGN_BONUS = 220.0;

    qreal best = std::numeric_limits<qreal>::max();
    bool found = false;
    for (NSide ss : allSides) {
        if (!m_from->hasOutput(ss)) continue;
        const QPointF sp = m_from->anchorScene(ss);
        for (NSide ts : allSides) {
            if (!m_to->hasInput(ts)) continue;
            const QPointF tp = m_to->anchorScene(ts);

            const QPointF v = tp - sp;                     // 源 → 目标 向量
            const qreal   dist = std::hypot(v.x(), v.y());
            if (dist < 1e-3) continue;
            const QPointF u(v.x() / dist, v.y() / dist);   // 单位方向

            const QPointF ds = sideDir(ss);                // 出端口朝外法向
            const QPointF dt = sideDir(ts);                // 入端口朝外法向
            // dot(ds, u)  : 出端口越朝向目标越大 (+1 最优)
            // dot(dt,-u)  : 入端口越迎向源越大  (+1 最优)
            const qreal alignOut = ds.x() * u.x() + ds.y() * u.y();
            const qreal alignIn  = dt.x() * (-u.x()) + dt.y() * (-u.y());

            // 出端口背离目标(<0)是肉眼可见的"绕回去"，狠狠惩罚。
            qreal cost = dist
                       - ALIGN_BONUS * alignOut
                       - ALIGN_BONUS * alignIn;
            if (alignOut < 0) cost += 400.0 * (-alignOut);
            if (alignIn  < 0) cost += 400.0 * (-alignIn);

            if (cost < best) {
                best = cost;
                m_startSide = toEdgeSide(ss);
                m_endSide   = toEdgeSide(ts);
                m_start     = sp;
                m_end       = tp;
                found = true;
            }
        }
    }
    if (!found) {
        // 兜底：直接用中心连
        m_start = m_from->anchorScene(NSide::Right);
        m_end   = m_to->anchorScene(NSide::Left);
        m_startSide = Right;
        m_endSide   = Left;
    }
}

void EdgeItem::recomputePath() const {
    pickEndpoints();
    if (!m_from || !m_to) { m_path = QPainterPath(); return; }
    if (g_style == Style::Orthogonal) recomputeOrthogonal();
    else                              recomputeBezier();
}

void EdgeItem::recomputeBezier() const {
    const qreal dx = m_end.x() - m_start.x();
    const qreal dy = m_end.y() - m_start.y();
    const qreal dist = std::sqrt(dx * dx + dy * dy);

    // 控制点伸长：与端口"垂直方向上的跨度"挂钩，再有最小值，
    // 这样水平对齐(dy≈0)的两节点之间也会画出明显的弧（之前会拍成直线）。
    auto sideOffset = [&](Side s) -> QPointF {
        switch (s) {
            case Right: case Left: {
                const qreal kx = qBound(60.0, qAbs(dx) * 0.5 + qAbs(dy) * 0.4, 240.0);
                return (s == Right) ? QPointF( kx, 0) : QPointF(-kx, 0);
            }
            case Top: case Bottom: {
                const qreal ky = qBound(60.0, qAbs(dy) * 0.5 + qAbs(dx) * 0.4, 240.0);
                return (s == Bottom) ? QPointF(0,  ky) : QPointF(0, -ky);
            }
        }
        return {};
    };
    Q_UNUSED(dist);
    const QPointF c1 = m_start + sideOffset(m_startSide);
    const QPointF c2 = m_end   + sideOffset(m_endSide);

    m_path = QPainterPath(m_start);
    m_path.cubicTo(c1, c2, m_end);
}

// 折线（直角）路由：每端先沿端口方向伸 kStub，再拐弯到对端 stub，再到锚点。
void EdgeItem::recomputeOrthogonal() const {
    auto stubOf = [](Side s) -> QPointF {
        switch (s) {
            case Right:  return QPointF( kStub, 0);
            case Left:   return QPointF(-kStub, 0);
            case Bottom: return QPointF(0,  kStub);
            case Top:    return QPointF(0, -kStub);
        }
        return {};
    };
    const QPointF s1 = m_start + stubOf(m_startSide);
    const QPointF e1 = m_end   + stubOf(m_endSide);

    m_path = QPainterPath(m_start);
    m_path.lineTo(s1);

    const bool startHorz = (m_startSide == Right || m_startSide == Left);
    const bool endHorz   = (m_endSide   == Right || m_endSide   == Left);

    if (startHorz && endHorz) {
        // 横向→横向：用中点列拐两次（同一行时两段重叠 → 视觉上即为直线）
        const qreal midX = (s1.x() + e1.x()) / 2.0;
        m_path.lineTo(QPointF(midX, s1.y()));
        m_path.lineTo(QPointF(midX, e1.y()));
        m_path.lineTo(e1);
    } else if (!startHorz && !endHorz) {
        // 纵向→纵向：用中点行拐两次
        const qreal midY = (s1.y() + e1.y()) / 2.0;
        m_path.lineTo(QPointF(s1.x(), midY));
        m_path.lineTo(QPointF(e1.x(), midY));
        m_path.lineTo(e1);
    } else if (startHorz && !endHorz) {
        // 横→纵：L 形（先到目标 stub 列，再下降）
        m_path.lineTo(QPointF(e1.x(), s1.y()));
        m_path.lineTo(e1);
    } else { // !startHorz && endHorz
        // 纵→横：L 形（先下降到目标 stub 行，再横移）
        m_path.lineTo(QPointF(s1.x(), e1.y()));
        m_path.lineTo(e1);
    }
    m_path.lineTo(m_end);
}

QRectF EdgeItem::boundingRect() const {
    recomputePath();
    if (m_path.isEmpty()) return {};
    return m_path.controlPointRect().adjusted(-12, -12, 12, 12);
}

QPainterPath EdgeItem::shape() const {
    recomputePath();
    QPainterPathStroker s;
    s.setWidth(10);
    return s.createStroke(m_path);
}

void EdgeItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    if (!m_from || !m_to) return;
    recomputePath();
    if (m_path.isEmpty()) return;

    QPen pen(QColor("#1976D2"), 2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawPath(m_path);

    // 终点箭头：沿末段切向
    const qreal t1 = (g_style == Style::Orthogonal) ? 0.999 : 0.985;
    const QPointF prev = m_path.pointAtPercent(t1);
    const QPointF tip  = m_end;
    QPointF dir = tip - prev;
    qreal len = std::hypot(dir.x(), dir.y());
    if (len < 1e-3) {
        // 折线最后一段太短时，按 endSide 方向取
        switch (m_endSide) {
            case Left:   dir = QPointF( 1, 0); break;
            case Right:  dir = QPointF(-1, 0); break;
            case Top:    dir = QPointF( 0, 1); break;
            case Bottom: dir = QPointF( 0,-1); break;
        }
        len = 1.0;
    }
    dir /= len;
    const QPointF n(-dir.y(), dir.x());
    const qreal sz = 7.0;
    QPolygonF arrow({
        tip,
        tip - dir * sz + n * (sz * 0.55),
        tip - dir * sz - n * (sz * 0.55)
    });
    p->setBrush(QColor("#1976D2"));
    p->setPen(Qt::NoPen);
    p->drawPolygon(arrow);
}
