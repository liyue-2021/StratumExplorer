#ifndef WORKFLOW_VIEW_H
#define WORKFLOW_VIEW_H

#include <QGraphicsView>

namespace processing
{
    namespace gui
    {

        /**
         * @brief 工作流画布视图
         *
         * 功能：
         *   - Ctrl + 滚轮缩放
         *   - Ctrl + +/- 缩放，Ctrl + 0 复位
         *   - 中键 / 空格 + 拖动 平移
         *   - 提供 zoomIn/zoomOut/zoomReset/zoomToFit 公开方法供工具栏调用
         */
        class WorkflowView : public QGraphicsView
        {
            Q_OBJECT
        public:
            explicit WorkflowView(QGraphicsScene *scene, QWidget *parent = nullptr);

        public slots:
            // 放大画布（缩放因子 1.2x）
            void zoomIn();
            // 缩小画布（缩放因子 1/1.2x）
            void zoomOut();
            // 恢复原始缩放比例 100%
            void zoomReset();
            // 自动调整缩放，使所有节点都在视口内
            void zoomToFit();

        protected:
            void wheelEvent(QWheelEvent *e) override;
            void keyPressEvent(QKeyEvent *e) override;

        private:
            void scaleBy(qreal factor);

            qreal m_minScale = 0.2;
            qreal m_maxScale = 4.0;
        };

    }
} // namespace processing::gui

#endif // WORKFLOW_VIEW_H
