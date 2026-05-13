// =============================================================================
//  PlotDialog.h
//  作者：工程师 ly
//
//  通用结果查看对话框（基于 QCustomPlot 渲染）。
//  - 双击工作流画布上的节点弹出本对话框
//  - display.* 类节点：调用 setSeries() / setDemoFromSeed() 显示曲线
//    （当前用 instanceId 作为种子产生确定性占位曲线，等真实管线接入后
//    只需把数据源换掉即可）
//  - 非 display 节点：调用 setMetaInfo() 在顶部显示输出文件路径/大小、
//    节点状态等元信息，画图区域留空，便于"跑完了到底产出了啥"的查看
//  - 内置 PNG / CSV 导出按钮
// =============================================================================
#ifndef PROCESSING_GUI_PLOT_DIALOG_H
#define PROCESSING_GUI_PLOT_DIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

class QCustomPlot;
class QComboBox;
class QPushButton;
class QLabel;

namespace processing { namespace gui {

/**
 * @brief 用 QCustomPlot 渲染上游节点输出的曲线。
 *
 * 现阶段：上游 Demo 节点没有真实数据，先用确定性的伪随机曲线占位
 * （以节点 instanceId 作为种子），保证不同节点显示不同结果，
 * 真实管线就绪后只需把 setSeries() 接到真实数据即可。
 */
class PlotDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlotDialog(const QString& title, QWidget* parent = nullptr);

    // 一组同 X 多 Y 的曲线；name 用于 legend
    void setSeries(const QVector<double>& x,
                   const QVector<QVector<double>>& ys,
                   const QStringList& names);

    // 用 instanceId 作种子生成一条占位演示曲线（512 点）。
    void setDemoFromSeed(const QString& seed, int curveCount = 1);
    // 顶部信息区：列出该节点的输出文件、参数摘要、耗时等。
    // 供 预处理/解释 类节点双击后查看“跑完了出了什么”。
    void setMetaInfo(const QStringList& outputFiles,
                     const QString& summary = {});
private slots:
    void onExportPng();
    void onExportCsv();

private:
    QCustomPlot* m_plot   = nullptr;
    QPushButton* m_btnPng = nullptr;
    QPushButton* m_btnCsv = nullptr;
    QLabel*      m_metaLabel = nullptr;

    QVector<double>          m_x;
    QVector<QVector<double>> m_ys;
    QStringList              m_names;
};

}} // namespace processing::gui

#endif
