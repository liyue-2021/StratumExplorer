#ifndef NODE_ITEM_H
#define NODE_ITEM_H

#include <QGraphicsObject>
#include <QPointer>
#include <QString>
#include <QColor>
#include <QSet>
#include <QSizeF>

#include "service/processing/IWorkflowEngine.h" // for NodeInstance
#include "processing/WorkflowTypes.h"

namespace processing
{
    namespace gui
    {

        class EdgeItem;

        /**
         * @brief 画布上的节点视图（自适应宽度的卡片）
         *
         * 视觉规范：
         *   - 标题 + 类型标识 + 状态文字
         *   - 4 向锚点（左/右/上/下）：左、上 = 输入侧；右、下 = 输出侧
         *     PureInput  只画左/上 黄色端口（节点本身只用于消费数据）
         *     PureOutput 只画右/下 绿色端口（节点本身只用于产出数据）
         *     InOut      左/上 + 右/下 都画
         *   - 启停按钮（▶ / ■）
         *   - 不同 NodeGroup 不同卡片底色
         *
         * 与 EdgeItem 关联（生命周期安全）：
         *   - EdgeItem 在构造时 attachEdge()、析构时 detachEdge()
         *   - m_edges 在迭代时跳过空 QPointer（节点已被删除的边）
         *   - 本类继承 QGraphicsObject，可在场景里用 QPointer<NodeItem> 弱引用
         */
        class NodeItem : public QGraphicsObject
        {
            Q_OBJECT
        public:
            enum class Side
            {
                Left,
                Right,
                Top,
                Bottom
            };

            explicit NodeItem(const processing::NodeInstance &info,
                              processing::NodeGroup group = processing::NodeGroup::Preprocess,
                              processing::NodeKind kind = processing::NodeKind::InOut);

            // 获取该节点的唯一实例 ID
            QString instanceId() const { return m_id; }
            // 获取该节点的类型 ID（如 "input.las", "preprocess.bandpass"）
            QString typeId() const { return m_typeId; }
            // 获取节点类型：纯输入/纯输出/双向
            processing::NodeKind kind() const { return m_kind; }
            // 设置节点运行状态（空闲/运行中/成功/失败/已停止等）
            void setStatus(processing::NodeStatus s);
            // 更新节点标题（用于重命名操作）
            void setTitle(const QString &title);
            /// 临时测试角标：甲方文档序号（如「8·DAS·B」），正式版可关闭
            void setTestSeqLabel(const QString &label);
            // 设置节点卡片的宽高（序列化还原时由 WorkflowScene 调用，范围有限制）
            void setSize(qreal w, qreal h);
            // 获取当前节点卡片尺寸
            QSizeF currentSize() const { return QSizeF(m_width, m_height); }

            // 获取该侧的锚点坐标（场景坐标系，用于连线端点）
            QPointF anchorScene(Side s) const;
            // 判断该侧是否为输入端口（供 EdgeItem 路由使用）
            bool hasInput(Side s) const;
            // 判断该侧是否为输出端口
            bool hasOutput(Side s) const;
            // 命中测试：检查本地坐标是否点在输入端口（local 为节点局部坐标）
            Side hitInputSide(const QPointF &local, bool *hit) const;
            // 命中测试：检查本地坐标是否点在输出端口
            Side hitOutputSide(const QPointF &local, bool *hit) const;

            // 注册关联的边（EdgeItem 构造时调用，用于移动通知）
            void attachEdge(EdgeItem *e);
            // 注销关联的边（EdgeItem 析构时调用）
            void detachEdge(EdgeItem *e);

            QRectF boundingRect() const override;
            void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override;

        protected:
            void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
            void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override;
            void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
            void hoverMoveEvent(QGraphicsSceneHoverEvent *e) override;
            void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
            QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

        private:
            void recomputeSize();
            QColor groupColor() const;
            QColor statusColor() const;

            QPointF portCenter(Side s) const;
            QRectF gripRect() const; // 右下 resize 拖柄 (local)

            QString m_id;
            QString m_title;
            QString m_typeId;
            QString m_testSeqLabel; ///< 画布角标，空则不绘制
            processing::NodeGroup m_group;
            processing::NodeKind m_kind;
            processing::NodeStatus m_status = processing::NodeStatus::Idle;

            qreal m_width = 200;
            qreal m_height = 78;
            qreal m_minWidth = 160; // 由 recomputeSize() 根据文本动态计算
            qreal m_minHeight = 78;
            qreal m_resizeStartX = 0;
            qreal m_resizeStartY = 0;
            qreal m_resizeStartW = 0;
            qreal m_resizeStartH = 0;

            bool m_resizing = false;
            bool m_dragging = false;

            bool m_hovering = false; // 鼠标是否在节点上

            // 关联的边：EdgeItem 构造/析构里会成对调 attachEdge/detachEdge，
            // 保证集合内不会出现已销毁的指针；NodeItem 自身被删时，关联的边
            // 由 WorkflowScene::onNodeRemoved 提前清理。
            QSet<EdgeItem *> m_edges;
        };

    }
} // namespace processing::gui

#endif // NODE_ITEM_H
