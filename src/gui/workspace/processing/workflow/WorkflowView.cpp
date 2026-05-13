#include "WorkflowView.h"

#include <QWheelEvent>
#include <QKeyEvent>

using namespace processing::gui;

WorkflowView::WorkflowView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // 鼠标中键拖动平移
    setInteractive(true);
    // 接受来自 NodePalette 的拖入（默认 QGraphicsView 的 viewport 不接受 drop）
    setAcceptDrops(true);
}

void WorkflowView::scaleBy(qreal factor)
{
    // 获取当前缩放比例，计算应用因子后的新比例
    const qreal cur = transform().m11();
    const qreal target = cur * factor;
    // 检查是否超过缩放范围限制（0.2 ~ 4.0）
    if (target < m_minScale || target > m_maxScale)
        return;
    // 应用缩放变换
    scale(factor, factor);
}

void WorkflowView::zoomIn() { scaleBy(1.2); }
void WorkflowView::zoomOut() { scaleBy(1.0 / 1.2); }
void WorkflowView::zoomReset() { resetTransform(); }

void WorkflowView::zoomToFit()
{
    if (!scene())
        return;
    fitInView(scene()->itemsBoundingRect().adjusted(-40, -40, 40, 40),
              Qt::KeepAspectRatio);
}

void WorkflowView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        const qreal step = (e->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
        scaleBy(step);
        e->accept();
    }
    else
    {
        QGraphicsView::wheelEvent(e);
    }
}

void WorkflowView::keyPressEvent(QKeyEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        switch (e->key())
        {
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomIn();
            e->accept();
            return;
        case Qt::Key_Minus:
            zoomOut();
            e->accept();
            return;
        case Qt::Key_0:
            zoomReset();
            e->accept();
            return;
        case Qt::Key_F:
            zoomToFit();
            e->accept();
            return;
        }
    }
    QGraphicsView::keyPressEvent(e);
}
