// =============================================================================
//  PlotDialog.h
//  作者：工程师 ly
//
//  结果查看对话框：全貌热力图 + 单通道曲线（真实 HDF5 / PlotData）
// =============================================================================
#ifndef PROCESSING_GUI_PLOT_DIALOG_H
#define PROCESSING_GUI_PLOT_DIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

class QMouseEvent;
class QTimer;

#include "qcustomplot.h"

class QCustomPlot;
class QCPColorMap;
class QCPColorScale;
class QPushButton;
class QLabel;
class QSpinBox;
class QTabWidget;

namespace processing { namespace gui {

class PlotDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlotDialog(const QString& title, QWidget* parent = nullptr);

    /// 通过 PlotData 读取 processed_*.h5（需 WITH_HDF5 构建）
    bool loadResultFromPlotService(const QString& h5Path, int funcId, QString* err = nullptr);

    void setMetaInfo(const QStringList& outputFiles,
                     const QString& summary = {});

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onChannelChanged(int channel);
    void onApplyRangeFilter();
    void onRangeFilterFull();
    void refreshInteractiveReplotStat();
    void onResetView();
    void onExportPng();
    void onExportCsv();

private:
    void setupUi();
    void showHeatmapTab(bool visible);
    qint64 rebuildHeatmap(double* outReplotMs = nullptr);
    void rebuildChannelCurve(int channelIndex);
    void updateFilterSpinRanges();
    void updateRangeFilterAxisLabels();
    void applyRangeFilterFromUi();
    void resetRangeFilterToFull();
    void columnIndexBounds(int& c0, int& c1) const;
    void channelRowBounds(int& r0, int& r1) const;
    int fullRowIndexForChannel(int channelNo) const;
    bool isFftPlot() const;
    int  keySpinMaximum() const;
    int  columnIndexFromKeySpin(int keySpinValue) const;
    void beginPlotRangeUpdate();
    void endPlotRangeUpdate();
    void configurePlotPerformance(QCustomPlot* plot);
    double replotMeasured(QCustomPlot* plot);
    void scheduleReplotStatsUpdate(QCustomPlot* plot);
    void cacheBuildStats(qint64 fillMs, double buildReplotMs);
    void refreshStatsLabel();
    void saveDefaultRanges(QCustomPlot* plot);
    void applyDefaultRanges(QCustomPlot* plot);
    void installPlotInteraction(QCustomPlot* plot);
    void clampAxisZoomOut(QCPAxis* axis, const QCPRange& dataRange);
    void updateHoverInfo(QCustomPlot* plot, QMouseEvent* event);
    QCustomPlot* activePlot() const;

    QTabWidget*    m_tabs         = nullptr;
    QCustomPlot*   m_plotHeat     = nullptr;
    QCustomPlot*   m_plotCurve    = nullptr;
    QCPColorMap*   m_colorMap     = nullptr;
    QCPColorScale* m_colorScale   = nullptr;
    QSpinBox*      m_channelSpin  = nullptr;
    QLabel*        m_channelLabel = nullptr;
    QLabel*        m_heatStatsLabel = nullptr;
    QWidget*       m_rangePanel   = nullptr;
    QLabel*        m_filterKeyLabel = nullptr;
    QSpinBox*      m_filterTStart = nullptr;
    QSpinBox*      m_filterTEnd   = nullptr;
    QSpinBox*      m_filterChStart = nullptr;
    QSpinBox*      m_filterChEnd  = nullptr;
    QPushButton*   m_btnApplyRange = nullptr;
    QPushButton*   m_btnRangeFull  = nullptr;
    QLabel*        m_metaLabel    = nullptr;
    QPushButton*   m_btnPng       = nullptr;
    QPushButton*   m_btnCsv       = nullptr;
    QPushButton*   m_btnReset     = nullptr;

    bool m_hasHeatmap = false;
    bool m_useRealH5 = false;
    int  m_plotFuncId = -1;
    QString m_loadedH5Path;
    QString m_loadedDatasetPath;
    bool m_updatingRange = false;
    qint64 m_cachedFillMs = -1;
    double m_cachedBuildReplotMs = -1.0;
    double m_cachedInteractReplotMs = -1.0;
    QTimer* m_replotStatsTimer = nullptr;
    QCustomPlot* m_statsPlot = nullptr;
    QCPRange m_defHeatX;
    QCPRange m_defHeatY;
    QCPRange m_defCurveX;
    QCPRange m_defCurveY;

    int m_fullHeatRows = 0;
    int m_fullHeatCols = 0;
    QVector<double> m_fullHeatX;
    QVector<double> m_fullHeatY;
    QVector<double> m_fullHeatZ;

    int m_heatRows = 0;
    int m_heatCols = 0;
    double m_origKeyMax = 30000.0;
    QVector<double> m_heatX;
    QVector<double> m_heatY;
    QVector<double> m_heatZ;

    QVector<double>          m_x;
    QVector<QVector<double>> m_ys;
    QStringList              m_names;
};

}} // namespace processing::gui

#endif
