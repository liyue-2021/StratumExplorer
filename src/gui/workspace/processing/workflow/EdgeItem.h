#ifndef EDGE_ITEM_H
#define EDGE_ITEM_H

#include <QGraphicsObject>
#include <QPointer>
#include <QString>

#include "NodeItem.h"

namespace processing { namespace gui {

/**
 * @brief DAG 边视图（曲线 / 折线 双模式）
 *
 * 自适应锚点：在源节点 (right, bottom) × 目标节点 (left, top) 共 4 候选中
 * 选择最短的一对。控制 / 转折点根据所选侧方向计算。
 *
 * 两种走线风格（全局静态切换）：
 *   - Style::Bezier      —— 三次贝塞尔曲线（默认）
 *   - Style::Orthogonal  —— 折线（直角拐弯，类钉钉/Visio）
 * 使用 setStyle / style() 静态接口；切换后由调用方触发场景重绘。
 *
 * 节点移动 / 大小变化：NodeItem::itemChange 会回调 onEndpointGeometryChanged()。
 *
 * 跨对象引用使用 QPointer<NodeItem>，节点先于本边被删除时能自动置空，
 * paint / shape 都做了空指针保护。
 */
class EdgeItem : public QGraphicsObject {
    Q_OBJECT
public:
    enum Side { Right, Bottom, Left, Top };
    enum class Style { Bezier, Orthogonal };

    static Style style();
    static void  setStyle(Style s);

    EdgeItem(NodeItem* from, const QString& fromPort,
             NodeItem* to,   const QString& toPort);
    ~EdgeItem() override;

    bool      matches(const QString& fromId, const QString& toId,
                      const QString& fromPort = {}, const QString& toPort = {}) const;
    bool      touches(NodeItem* node) const;
    NodeItem* fromNode() const { return m_from.data(); }
    NodeItem* toNode()   const { return m_to.data();   }

    void onEndpointGeometryChanged();

    QRectF        boundingRect() const override;
    QPainterPath  shape()        const override;
    void          paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
    void recomputePath()       const;
    void recomputeBezier()     const;
    void recomputeOrthogonal() const;
    void pickEndpoints()       const;   // 选 startSide/endSide 与 m_start/m_end

    QPointer<NodeItem> m_from;
    QPointer<NodeItem> m_to;
    QString            m_fromPort;
    QString            m_toPort;

    mutable QPainterPath m_path;
    mutable QPointF      m_start;
    mutable QPointF      m_end;
    mutable Side         m_startSide = Right;
    mutable Side         m_endSide   = Left;
};

}} // namespace processing::gui

#endif // EDGE_ITEM_H
