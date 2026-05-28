#include "PlotDialog.h"

#include "PlotData.h"

#include "qcustomplot.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QToolTip>
#include <QProgressDialog>
#include <QPushButton>
#include <QSizePolicy>
#include <QTabWidget>
#include <QTimer>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>
#include <algorithm>
#include <limits>

using namespace processing::gui;

namespace {

const QVector<QColor> kPalette = {
    QColor("#1976D2"), QColor("#E64A19"), QColor("#388E3C"),
    QColor("#7B1FA2"), QColor("#F57C00"), QColor("#0097A7"),
};

constexpr double kZoomMaxSpanFrac = 1.02;

int nearestAxisIndex(const QVector<double>& axis, double v)
{
    if (axis.isEmpty())
        return -1;
    auto it = std::lower_bound(axis.begin(), axis.end(), v);
    int idx = static_cast<int>(it - axis.begin());
    if (idx >= axis.size())
        idx = axis.size() - 1;
    if (idx > 0) {
        const double dPrev = qAbs(axis[idx - 1] - v);
        const double dCur = qAbs(axis[idx] - v);
        if (dPrev < dCur)
            --idx;
    }
    return idx;
}

void configurePlotPerformance(QCustomPlot* plot)
{
    if (!plot)
        return;
    plot->setAttribute(Qt::WA_OpaquePaintEvent, true);
    plot->setPlottingHint(QCP::phFastPolylines, true);
    plot->setNoAntialiasingOnDrag(true);
    plot->setNotAntialiasedElements(QCP::aePlottables | QCP::aeGrid);
    plot->setOpenGl(true, 16);
}

} // namespace

PlotDialog::PlotDialog(const QString& title, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setMinimumSize(720, 480);
    resize(1000, 640);
    setupUi();
}

void PlotDialog::setupUi()
{
    m_metaLabel = new QLabel(this);
    m_metaLabel->setStyleSheet(QStringLiteral("background:#ECEFF1; color:#37474F; padding:6px;"));
    m_metaLabel->setWordWrap(true);
    m_metaLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_metaLabel->hide();

    m_plotHeat = new QCustomPlot(this);
    m_plotHeat->setMinimumHeight(280);
    m_plotHeat->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_colorMap = new QCPColorMap(m_plotHeat->xAxis, m_plotHeat->yAxis);
    m_colorScale = new QCPColorScale(m_plotHeat);
    m_plotHeat->plotLayout()->addElement(0, 1, m_colorScale);
    m_colorMap->setColorScale(m_colorScale);
    m_colorMap->setGradient(QCPColorGradient::gpJet);
    m_colorMap->setInterpolate(true);
    m_colorScale->axis()->setLabel(tr("幅值"));
    configurePlotPerformance(m_plotHeat);

    m_plotCurve = new QCustomPlot(this);
    m_plotCurve->setMinimumHeight(280);
    m_plotCurve->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                                  QCP::iSelectAxes | QCP::iSelectLegend |
                                  QCP::iSelectPlottables);
    m_plotCurve->legend->setVisible(false);
    m_plotCurve->axisRect()->setupFullAxesBox(true);
    configurePlotPerformance(m_plotCurve);

    m_rangePanel = new QWidget(this);
    auto* rangeLay = new QHBoxLayout(m_rangePanel);
    rangeLay->setContentsMargins(0, 0, 0, 4);
    auto* lblRange = new QLabel(tr("范围筛选"), m_rangePanel);
    lblRange->setStyleSheet(QStringLiteral("font-weight:600; color:#37474F;"));
    m_filterKeyLabel = new QLabel(tr("采样点"), m_rangePanel);
    m_filterTStart = new QSpinBox(m_rangePanel);
    m_filterTEnd = new QSpinBox(m_rangePanel);
    auto* lblTilde1 = new QLabel(QStringLiteral("~"), m_rangePanel);
    auto* lblCh = new QLabel(tr("通道"), m_rangePanel);
    m_filterChStart = new QSpinBox(m_rangePanel);
    m_filterChEnd = new QSpinBox(m_rangePanel);
    auto* lblTilde2 = new QLabel(QStringLiteral("~"), m_rangePanel);
    m_btnApplyRange = new QPushButton(tr("应用"), m_rangePanel);
    m_btnRangeFull = new QPushButton(tr("全范围"), m_rangePanel);
    for (QSpinBox* sb : {m_filterTStart, m_filterTEnd, m_filterChStart, m_filterChEnd}) {
        sb->setMinimumWidth(72);
        sb->setAccelerated(true);
    }
    connect(m_btnApplyRange, &QPushButton::clicked, this, &PlotDialog::onApplyRangeFilter);
    connect(m_btnRangeFull, &QPushButton::clicked, this, &PlotDialog::onRangeFilterFull);
    rangeLay->addStretch();
    rangeLay->addWidget(lblRange);
    rangeLay->addSpacing(8);
    rangeLay->addWidget(m_filterKeyLabel);
    rangeLay->addWidget(m_filterTStart);
    rangeLay->addWidget(lblTilde1);
    rangeLay->addWidget(m_filterTEnd);
    rangeLay->addSpacing(12);
    rangeLay->addWidget(lblCh);
    rangeLay->addWidget(m_filterChStart);
    rangeLay->addWidget(lblTilde2);
    rangeLay->addWidget(m_filterChEnd);
    rangeLay->addSpacing(8);
    rangeLay->addWidget(m_btnApplyRange);
    rangeLay->addWidget(m_btnRangeFull);

    m_btnReset = new QPushButton(tr("复位视图"), m_rangePanel);
    m_btnReset->setToolTip(tr("恢复当前图的默认缩放与平移"));
    connect(m_btnReset, &QPushButton::clicked, this, &PlotDialog::onResetView);
    rangeLay->addSpacing(12);
    rangeLay->addWidget(m_btnReset);

    m_rangePanel->hide();

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(m_plotHeat, tr("全貌热力图"));
    m_tabs->addTab(m_plotCurve, tr("单通道曲线"));

    m_channelLabel = new QLabel(tr("通道编号："), this);
    m_channelSpin = new QSpinBox(this);
    m_channelSpin->setRange(0, 0);
    m_channelSpin->setValue(0);
    m_channelSpin->setToolTip(
        tr("用于「单通道曲线」页：选择要查看的通道编号，切换后刷新该通道的曲线。"));
    connect(m_channelSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PlotDialog::onChannelChanged);

    m_heatStatsLabel = new QLabel(this);
    m_heatStatsLabel->setStyleSheet(QStringLiteral("color:#546E7A; font-size:11px;"));
    m_heatStatsLabel->hide();

    auto* channelBar = new QHBoxLayout;
    channelBar->addWidget(m_channelLabel);
    channelBar->addWidget(m_channelSpin);
    channelBar->addStretch();
    channelBar->addWidget(m_heatStatsLabel);

    m_btnPng = new QPushButton(tr("导出 PNG…"), this);
    m_btnCsv = new QPushButton(tr("导出 CSV…"), this);
    auto* btnClose = new QPushButton(tr("关闭"), this);
    connect(m_btnPng, &QPushButton::clicked, this, &PlotDialog::onExportPng);
    connect(m_btnCsv, &QPushButton::clicked, this, &PlotDialog::onExportCsv);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    auto* btns = new QHBoxLayout;
    btns->addLayout(channelBar);
    btns->addStretch();
    btns->addWidget(m_btnPng);
    btns->addWidget(m_btnCsv);
    btns->addWidget(btnClose);

    m_plotHeat->installEventFilter(this);
    m_plotCurve->installEventFilter(this);
    installPlotInteraction(m_plotHeat);
    installPlotInteraction(m_plotCurve);

    auto* root = new QVBoxLayout(this);
    root->addWidget(m_metaLabel);
    root->addWidget(m_rangePanel);
    root->addWidget(m_tabs, 1);
    root->addLayout(btns);

    showHeatmapTab(false);
}

void PlotDialog::showHeatmapTab(bool visible)
{
    m_hasHeatmap = visible;
    if (!m_tabs)
        return;
    m_tabs->setTabVisible(0, visible);
    if (!visible)
        m_tabs->setCurrentWidget(m_plotCurve);
    m_channelLabel->setVisible(visible);
    m_channelSpin->setVisible(visible);
    m_heatStatsLabel->setVisible(visible);
    if (m_rangePanel)
        m_rangePanel->setVisible(visible);
    if (m_btnReset)
        m_btnReset->setVisible(visible);
}

void PlotDialog::setMetaInfo(const QStringList& outputFiles, const QString& summary)
{
    QStringList lines;
    if (!summary.isEmpty())
        lines << summary;
    if (outputFiles.isEmpty()) {
        lines << tr("该节点未产生输出文件。");
    } else {
        for (const QString& f : outputFiles) {
            QFileInfo fi(f);
            if (fi.exists()) {
                lines << QStringLiteral("输出: %1   (%2 KB)")
                             .arg(fi.absoluteFilePath())
                             .arg(fi.size() / 1024);
            } else {
                lines << QStringLiteral("输出(待生成): %1").arg(f);
            }
        }
    }
    m_metaLabel->setText(lines.join(QLatin1Char('\n')));
    m_metaLabel->setVisible(true);
}

void PlotDialog::beginPlotRangeUpdate()
{
    m_updatingRange = true;
}

void PlotDialog::endPlotRangeUpdate()
{
    m_updatingRange = false;
}

bool PlotDialog::isFftPlot() const
{
    return m_plotFuncId == 8;
}

int PlotDialog::keySpinMaximum() const
{
    if (isFftPlot()) {
        if (!m_fullHeatX.isEmpty())
            return qMax(1, static_cast<int>(qRound(m_fullHeatX.last())));
        return qMax(1, static_cast<int>(qRound(m_origKeyMax)));
    }
    return qMax(0, m_fullHeatCols - 1);
}

int PlotDialog::columnIndexFromKeySpin(int keySpinValue) const
{
    if (!isFftPlot())
        return qBound(0, keySpinValue, qMax(0, m_fullHeatCols - 1));
    if (m_fullHeatX.isEmpty())
        return 0;
    const int idx = nearestAxisIndex(m_fullHeatX, static_cast<double>(keySpinValue));
    return qBound(0, idx, m_fullHeatCols - 1);
}

void PlotDialog::updateRangeFilterAxisLabels()
{
    const bool isFft = isFftPlot();
    if (m_filterKeyLabel)
        m_filterKeyLabel->setText(isFft ? tr("频率 (Hz)") : tr("采样点"));
    const QString hzSuffix = isFft ? QStringLiteral(" Hz") : QString();
    if (m_filterTStart)
        m_filterTStart->setSuffix(hzSuffix);
    if (m_filterTEnd)
        m_filterTEnd->setSuffix(hzSuffix);
    if (m_btnApplyRange) {
        m_btnApplyRange->setToolTip(
            isFft ? tr("按频率 (Hz) 与通道编号裁剪热力图与单通道曲线")
                  : tr("按采样点列索引与通道编号裁剪显示（0 起）"));
    }
    if (m_filterTStart) {
        m_filterTStart->setToolTip(
            isFft ? tr("起始频率 (Hz)，与热力图横轴一致")
                  : tr("采样点起始列索引（含），0 为第一个采样点"));
    }
    if (m_filterTEnd) {
        m_filterTEnd->setToolTip(
            isFft ? tr("结束频率 (Hz)，与热力图横轴一致")
                  : tr("采样点结束列索引（含）"));
    }
    if (m_btnRangeFull) {
        m_btnRangeFull->setToolTip(
            isFft ? tr("恢复为全部频率与通道") : tr("恢复为全部采样点与通道"));
    }
    if (m_channelSpin) {
        m_channelSpin->setToolTip(
            isFft ? tr("「单通道曲线」页：选择通道后显示该通道的频率-幅值曲线")
                  : tr("「单通道曲线」页：选择通道后显示该通道的采样点-幅值曲线"));
    }
}

void PlotDialog::updateFilterSpinRanges()
{
    if (!m_filterTStart || m_fullHeatCols <= 0)
        return;

    const int keyMax = keySpinMaximum();
    const int lastCh = m_fullHeatRows > 0
                           ? static_cast<int>(m_fullHeatY.last())
                           : qMax(0, m_fullHeatRows - 1);

    m_filterTStart->setRange(0, keyMax);
    m_filterTEnd->setRange(0, keyMax);
    m_filterChStart->setRange(0, lastCh);
    m_filterChEnd->setRange(0, lastCh);
}

void PlotDialog::columnIndexBounds(int& c0, int& c1) const
{
    int t0 = m_filterTStart->value();
    int t1 = m_filterTEnd->value();
    if (t0 > t1)
        std::swap(t0, t1);
    c0 = columnIndexFromKeySpin(t0);
    c1 = columnIndexFromKeySpin(t1);
    if (c0 > c1)
        std::swap(c0, c1);
}

void PlotDialog::channelRowBounds(int& r0, int& r1) const
{
    int ch0 = m_filterChStart->value();
    int ch1 = m_filterChEnd->value();
    if (ch0 > ch1)
        std::swap(ch0, ch1);
    const int ri0 = fullRowIndexForChannel(ch0);
    const int ri1 = fullRowIndexForChannel(ch1);
    if (ri0 < 0 || ri1 < 0) {
        r0 = -1;
        r1 = -1;
        return;
    }
    r0 = qMin(ri0, ri1);
    r1 = qMax(ri0, ri1);
}

void PlotDialog::resetRangeFilterToFull()
{
    if (!m_filterTStart || m_fullHeatCols <= 0)
        return;

    updateFilterSpinRanges();
    const int keyMax = keySpinMaximum();
    const int lastCh = m_fullHeatRows > 0
                           ? static_cast<int>(m_fullHeatY.last())
                           : qMax(0, m_fullHeatRows - 1);

    m_filterTStart->blockSignals(true);
    m_filterTEnd->blockSignals(true);
    m_filterChStart->blockSignals(true);
    m_filterChEnd->blockSignals(true);
    m_filterTStart->setValue(0);
    m_filterTEnd->setValue(keyMax);
    m_filterChStart->setValue(0);
    m_filterChEnd->setValue(lastCh);
    m_filterTStart->blockSignals(false);
    m_filterTEnd->blockSignals(false);
    m_filterChStart->blockSignals(false);
    m_filterChEnd->blockSignals(false);

    applyRangeFilterFromUi();
}

int PlotDialog::fullRowIndexForChannel(int channelNo) const
{
    if (m_fullHeatRows <= 0)
        return -1;
    for (int r = 0; r < m_fullHeatRows; ++r) {
        if (static_cast<int>(m_fullHeatY[r]) == channelNo)
            return r;
    }
    if (channelNo >= 0 && channelNo < m_fullHeatRows)
        return channelNo;
    return -1;
}

void PlotDialog::applyRangeFilterFromUi()
{
    if (m_fullHeatCols <= 0 || m_fullHeatRows <= 0)
        return;

    int c0 = 0;
    int c1 = 0;
    int row0 = 0;
    int row1 = 0;
    columnIndexBounds(c0, c1);
    channelRowBounds(row0, row1);
    if (row0 < 0 || row1 < 0) {
        QMessageBox::warning(this, tr("范围筛选"), tr("通道范围无效。"));
        return;
    }

    const int ch0 = static_cast<int>(m_fullHeatY[row0]);
    const int ch1 = static_cast<int>(m_fullHeatY[row1]);

    m_heatCols = c1 - c0 + 1;
    m_heatRows = row1 - row0 + 1;
    m_heatX.resize(m_heatCols);
    m_heatY.resize(m_heatRows);
    m_heatZ.resize(static_cast<int>(m_heatRows) * m_heatCols);

    for (int c = 0; c < m_heatCols; ++c)
        m_heatX[c] = m_fullHeatX[c0 + c];
    for (int r = 0; r < m_heatRows; ++r) {
        m_heatY[r] = m_fullHeatY[row0 + r];
        const int srcRow = row0 + r;
        for (int c = 0; c < m_heatCols; ++c) {
            m_heatZ[r * m_heatCols + c] =
                m_fullHeatZ[srcRow * m_fullHeatCols + c0 + c];
        }
    }

    m_channelSpin->blockSignals(true);
    m_channelSpin->setRange(ch0, ch1);
    if (m_channelSpin->value() < ch0 || m_channelSpin->value() > ch1)
        m_channelSpin->setValue(ch0);
    m_channelSpin->blockSignals(false);

    double buildReplotMs = 0.0;
    const qint64 fillMs = rebuildHeatmap(&buildReplotMs);
    rebuildChannelCurve(m_channelSpin->value());
    cacheBuildStats(fillMs, buildReplotMs);
    refreshStatsLabel();
}

void PlotDialog::onApplyRangeFilter()
{
    if (!m_hasHeatmap)
        return;
    applyRangeFilterFromUi();
}

void PlotDialog::onRangeFilterFull()
{
    if (!m_hasHeatmap)
        return;
    resetRangeFilterToFull();
}

qint64 PlotDialog::rebuildHeatmap(double* outReplotMs)
{
    if (!m_colorMap || m_heatCols <= 0 || m_heatRows <= 0)
        return 0;

    QElapsedTimer fillTimer;
    fillTimer.start();

    auto* data = m_colorMap->data();
    const QCPRange xRange(m_heatX.first(), m_heatX.last());
    const QCPRange yRange(m_heatY.first(), m_heatY.last());
    m_defHeatX = xRange;
    m_defHeatY = yRange;

    beginPlotRangeUpdate();
    data->setSize(m_heatCols, m_heatRows);
    data->setRange(xRange, yRange);

    const bool showProgress =
        (static_cast<qint64>(m_heatRows) * m_heatCols >= 2000000);
    QProgressDialog progress;
    if (showProgress) {
        progress.setWindowTitle(tr("热力图"));
        progress.setLabelText(tr("正在写入 QCustomPlot 色图 (%1×%2)…")
                                  .arg(m_heatCols)
                                  .arg(m_heatRows));
        progress.setRange(0, m_heatRows);
        progress.setMinimumDuration(0);
        progress.setValue(0);
        progress.show();
        QApplication::processEvents();
    }

    for (int r = 0; r < m_heatRows; ++r) {
        if (showProgress && progress.wasCanceled())
            break;
        const int rowOff = r * m_heatCols;
        for (int c = 0; c < m_heatCols; ++c) {
            data->setCell(c, r, m_heatZ[rowOff + c]);
        }
        if (showProgress && (r % 8 == 0 || r == m_heatRows - 1)) {
            progress.setValue(r + 1);
            QApplication::processEvents();
        }
    }

    m_colorMap->rescaleDataRange(true);
    m_plotHeat->xAxis->setRange(xRange);
    m_plotHeat->yAxis->setRange(yRange);
    endPlotRangeUpdate();
    const double replotMs = replotMeasured(m_plotHeat);
    if (outReplotMs)
        *outReplotMs = replotMs;
    return fillTimer.elapsed();
}

void PlotDialog::rebuildChannelCurve(int channelIndex)
{
    if (m_fullHeatCols <= 0 || m_fullHeatRows <= 0)
        return;

    const int srcRow = fullRowIndexForChannel(channelIndex);
    if (srcRow < 0)
        return;

    int c0 = 0;
    int c1 = 0;
    columnIndexBounds(c0, c1);
    const int nOut = c1 - c0 + 1;
    if (nOut <= 0)
        return;

    QVector<double> x(nOut);
    QVector<double> y(nOut);
    for (int i = 0; i < nOut; ++i) {
        const int col = c0 + i;
        x[i] = m_fullHeatX[col];
        y[i] = m_fullHeatZ[srcRow * m_fullHeatCols + col];
    }

    m_x = x;
    m_ys = {y};
    m_names = {tr("通道 %1").arg(channelIndex)};

    m_plotCurve->clearGraphs();
    auto* g = m_plotCurve->addGraph();
    QPen pen(kPalette[0]);
    pen.setWidthF(1.4);
    pen.setCosmetic(true);
    g->setPen(pen);
    g->setAdaptiveSampling(true);
    g->setLineStyle(QCPGraph::lsLine);
    g->setScatterStyle(QCPScatterStyle::ssNone);
    g->setData(x, y, true);
    const QCPRange xRange(x.first(), x.last());
    double yMin = *std::min_element(y.begin(), y.end());
    double yMax = *std::max_element(y.begin(), y.end());
    const double pad = (yMax - yMin) * 0.08 + 1.0;
    const QCPRange yRange(yMin - pad, yMax + pad);
    m_defCurveX = xRange;
    m_defCurveY = yRange;
    beginPlotRangeUpdate();
    m_plotCurve->xAxis->setRange(xRange);
    m_plotCurve->yAxis->setRange(yRange);
    endPlotRangeUpdate();
    replotMeasured(m_plotCurve);
}

void PlotDialog::configurePlotPerformance(QCustomPlot* plot)
{
    ::configurePlotPerformance(plot);
}

double PlotDialog::replotMeasured(QCustomPlot* plot)
{
    if (!plot)
        return 0.0;
    plot->replot(QCustomPlot::rpRefreshHint);
    return plot->replotTime();
}

void PlotDialog::scheduleReplotStatsUpdate(QCustomPlot* plot)
{
    m_statsPlot = plot;
    if (!m_replotStatsTimer) {
        m_replotStatsTimer = new QTimer(this);
        m_replotStatsTimer->setSingleShot(true);
        m_replotStatsTimer->setInterval(350);
        connect(m_replotStatsTimer, &QTimer::timeout, this,
                &PlotDialog::refreshInteractiveReplotStat);
    }
    m_replotStatsTimer->start();
}

void PlotDialog::refreshInteractiveReplotStat()
{
    if (!m_statsPlot)
        return;
    m_cachedInteractReplotMs = m_statsPlot->replotTime(true);
    refreshStatsLabel();
}

void PlotDialog::saveDefaultRanges(QCustomPlot* plot)
{
    if (!plot)
        return;
    if (plot == m_plotHeat) {
        m_defHeatX = plot->xAxis->range();
        m_defHeatY = plot->yAxis->range();
    } else if (plot == m_plotCurve) {
        m_defCurveX = plot->xAxis->range();
        m_defCurveY = plot->yAxis->range();
    }
}

void PlotDialog::applyDefaultRanges(QCustomPlot* plot)
{
    if (!plot)
        return;
    m_updatingRange = true;
    if (plot == m_plotHeat) {
        plot->xAxis->setRange(m_defHeatX);
        plot->yAxis->setRange(m_defHeatY);
    } else if (plot == m_plotCurve) {
        plot->xAxis->setRange(m_defCurveX);
        plot->yAxis->setRange(m_defCurveY);
    }
    plot->replot();
    m_updatingRange = false;
}

void PlotDialog::clampAxisZoomOut(QCPAxis* axis, const QCPRange& dataRange)
{
    if (!axis || m_updatingRange || dataRange.size() <= 0.0)
        return;

    QCPRange r = axis->range();
    const double dataSpan = dataRange.size();
    const double maxSpan = dataSpan * kZoomMaxSpanFrac;
    const double margin = dataSpan * 0.02;

    bool changed = false;
    if (r.size() > maxSpan) {
        const double c = r.center();
        r.lower = c - maxSpan * 0.5;
        r.upper = c + maxSpan * 0.5;
        changed = true;
    }
    if (r.lower < dataRange.lower - margin) {
        r.upper += dataRange.lower - margin - r.lower;
        r.lower = dataRange.lower - margin;
        changed = true;
    }
    if (r.upper > dataRange.upper + margin) {
        r.lower -= r.upper - (dataRange.upper + margin);
        r.upper = dataRange.upper + margin;
        changed = true;
    }

    if (changed) {
        m_updatingRange = true;
        axis->setRange(r);
        m_updatingRange = false;
    }
}

void PlotDialog::installPlotInteraction(QCustomPlot* plot)
{
    if (!plot)
        return;
    plot->setMouseTracking(true);
    auto onRangeChanged = [this, plot](const QCPRange&) {
        if (m_updatingRange)
            return;
        if (plot == m_plotHeat) {
            if (m_heatCols > 0 && m_defHeatX.size() > 0.0) {
                clampAxisZoomOut(plot->xAxis, m_defHeatX);
                clampAxisZoomOut(plot->yAxis, m_defHeatY);
            }
        } else if (!m_x.isEmpty() && m_defCurveX.size() > 0.0) {
            clampAxisZoomOut(plot->xAxis, m_defCurveX);
            clampAxisZoomOut(plot->yAxis, m_defCurveY);
        }
        plot->replot(QCustomPlot::rpQueuedReplot);
        scheduleReplotStatsUpdate(plot);
    };
    connect(plot->xAxis,
            QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged),
            this, onRangeChanged);
    connect(plot->yAxis,
            QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged),
            this, onRangeChanged);
    connect(plot, &QCustomPlot::mouseMove, this,
            [this, plot](QMouseEvent* ev) { updateHoverInfo(plot, ev); });
}

bool PlotDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Leave
        && (watched == m_plotHeat || watched == m_plotCurve)) {
        QToolTip::hideText();
    }
    return QDialog::eventFilter(watched, event);
}

void PlotDialog::updateHoverInfo(QCustomPlot* plot, QMouseEvent* event)
{
    if (!plot || !event)
        return;

    if (!plot->axisRect()->rect().contains(event->pos())) {
        QToolTip::hideText();
        return;
    }

    const double key = plot->xAxis->pixelToCoord(event->pos().x());
    const double val = plot->yAxis->pixelToCoord(event->pos().y());
    QString tip;

    if (plot == m_plotHeat && m_hasHeatmap && m_heatCols > 0 && m_heatRows > 0) {
        const int col = nearestAxisIndex(m_heatX, key);
        const int row = nearestAxisIndex(m_heatY, val);
        if (col < 0 || row < 0)
            return;

        const double z = m_heatZ[row * m_heatCols + col];
        const QString xLabel = (m_plotFuncId == 8) ? tr("频率 (Hz)") : tr("采样点");
        tip = tr("%1: %2\n通道: %3\n幅值: %4")
                  .arg(xLabel)
                  .arg(m_heatX[col], 0, 'f', 2)
                  .arg(static_cast<int>(m_heatY[row]))
                  .arg(z, 0, 'f', 2);
    } else if (plot == m_plotCurve && !m_x.isEmpty() && !m_ys.isEmpty()) {
        const int idx = nearestAxisIndex(m_x, key);
        if (idx < 0)
            return;

        QStringList lines;
        const QString xLabel = m_plotCurve->xAxis->label().isEmpty()
                                   ? QStringLiteral("x")
                                   : m_plotCurve->xAxis->label();
        lines << tr("%1: %2").arg(xLabel).arg(m_x[idx], 0, 'f', 4);
        for (int s = 0; s < m_ys.size(); ++s) {
            if (idx >= m_ys[s].size())
                continue;
            const QString seriesName =
                (s < m_names.size() && !m_names[s].isEmpty())
                    ? m_names[s]
                    : tr("曲线 %1").arg(s + 1);
            lines << tr("%1: %2")
                         .arg(seriesName)
                         .arg(m_ys[s][idx], 0, 'f', 4);
        }
        tip = lines.join(QLatin1Char('\n'));
    }

    if (!tip.isEmpty()) {
        QToolTip::showText(plot->mapToGlobal(event->pos()), tip, plot);
    }
}

void PlotDialog::onResetView()
{
    QCustomPlot* plot = activePlot();
    if (!plot)
        return;
    if (plot == m_plotHeat && m_heatCols <= 0)
        return;
    if (plot == m_plotCurve && m_x.isEmpty())
        return;
    applyDefaultRanges(plot);
}

void PlotDialog::cacheBuildStats(qint64 fillMs, double buildReplotMs)
{
    if (fillMs >= 0)
        m_cachedFillMs = fillMs;
    if (buildReplotMs >= 0.0)
        m_cachedBuildReplotMs = buildReplotMs;
    m_cachedInteractReplotMs = -1.0;
}

void PlotDialog::refreshStatsLabel()
{
    if (!m_heatStatsLabel)
        return;

    QString text;
    if (m_hasHeatmap && m_heatCols > 0) {
        const qint64 cells = static_cast<qint64>(m_heatRows) * m_heatCols;
        text = tr("显示 %1×%2（%3 万点）")
                   .arg(m_heatCols)
                   .arg(m_heatRows)
                   .arg(cells / 10000.0, 0, 'f', 1);
        if (m_fullHeatCols > 0 && m_fullHeatRows > 0
            && (m_heatCols != m_fullHeatCols || m_heatRows != m_fullHeatRows)) {
            text += tr(" / 全量 %1×%2").arg(m_fullHeatCols).arg(m_fullHeatRows);
        }
        if (m_useRealH5) {
            text += tr(" · 真实数据");
            if (!m_loadedDatasetPath.isEmpty())
                text += tr(" · %1").arg(m_loadedDatasetPath);
        }
    } else if (!m_x.isEmpty()) {
        text = tr("曲线 %1 点").arg(m_x.size());
    }

    if (m_cachedFillMs >= 0)
        text += tr(" · 灌数据 %1s").arg(m_cachedFillMs / 1000.0, 0, 'f', 2);
    if (m_cachedBuildReplotMs >= 0.0)
        text += tr(" · 首绘 %1ms").arg(m_cachedBuildReplotMs, 0, 'f', 1);
    if (m_cachedInteractReplotMs >= 0.0)
        text += tr(" · 缩放绘制 %1ms").arg(m_cachedInteractReplotMs, 0, 'f', 1);

    if (!text.isEmpty())
        m_heatStatsLabel->setText(text);
}

bool PlotDialog::loadResultFromPlotService(const QString& h5Path, int funcId, QString* err)
{
    PlotRequest req;
    req.h5Path = h5Path;
    req.kind = QStringLiteral("Overview");

    PlotDisplayData plotOut;
    QString         loadErr;
    PlotData().LoadPlotData(req, &plotOut, &loadErr);
    if (!loadErr.isEmpty()) {
        if (err)
            *err = loadErr;
        return false;
    }
    if (plotOut.data.empty() || plotOut.rows <= 0 || plotOut.cols <= 0) {
        if (err)
            *err = tr("PlotData 返回空矩阵");
        return false;
    }

    const bool isFft = (funcId == 8);
    m_useRealH5 = true;
    m_plotFuncId = funcId;
    m_loadedH5Path = h5Path;
    m_loadedDatasetPath = QStringLiteral("/Processed/Data");
    showHeatmapTab(true);

    m_fullHeatRows = plotOut.rows;
    m_fullHeatCols = plotOut.cols;
    m_fullHeatZ.resize(static_cast<int>(plotOut.data.size()));
    for (int i = 0; i < m_fullHeatZ.size(); ++i)
        m_fullHeatZ[i] = static_cast<double>(plotOut.data[static_cast<size_t>(i)]);

    m_fullHeatX.resize(m_fullHeatCols);
    m_fullHeatY.resize(m_fullHeatRows);
    m_origKeyMax = isFft ? 250.0 : static_cast<double>(qMax(1, m_fullHeatCols));
    for (int c = 0; c < m_fullHeatCols; ++c)
        m_fullHeatX[c] = (c + 0.5) * m_origKeyMax / qMax(1, m_fullHeatCols);
    for (int r = 0; r < m_fullHeatRows; ++r)
        m_fullHeatY[r] = static_cast<double>(r);

    if (isFft) {
        m_plotHeat->xAxis->setLabel(tr("频率 (Hz)"));
        m_plotCurve->xAxis->setLabel(tr("频率 (Hz)"));
    } else {
        m_plotHeat->xAxis->setLabel(tr("采样点"));
        m_plotCurve->xAxis->setLabel(tr("采样点"));
    }
    m_plotHeat->yAxis->setLabel(tr("通道编号"));
    m_plotCurve->yAxis->setLabel(tr("幅值"));

    m_channelSpin->blockSignals(true);
    m_channelSpin->setRange(0, qMax(0, m_fullHeatRows - 1));
    m_channelSpin->setValue(m_fullHeatRows / 2);
    m_channelSpin->blockSignals(false);

    updateRangeFilterAxisLabels();
    updateFilterSpinRanges();
    resetRangeFilterToFull();
    m_tabs->setCurrentIndex(0);
    return true;
}

void PlotDialog::onChannelChanged(int channel)
{
    if (!m_hasHeatmap)
        return;
    rebuildChannelCurve(channel);
}

QCustomPlot* PlotDialog::activePlot() const
{
    if (!m_tabs)
        return m_plotCurve;
    return (m_tabs->currentWidget() == m_plotHeat) ? m_plotHeat : m_plotCurve;
}

void PlotDialog::onExportPng()
{
    const QString f = QFileDialog::getSaveFileName(
        this, tr("导出 PNG"), QString(), tr("PNG 图像 (*.png)"));
    if (f.isEmpty())
        return;

    QCustomPlot* plot = activePlot();
    QVector<bool> adaptiveBackup;
    if (plot == m_plotCurve) {
        adaptiveBackup.resize(plot->graphCount());
        for (int i = 0; i < plot->graphCount(); ++i) {
            adaptiveBackup[i] = plot->graph(i)->adaptiveSampling();
            plot->graph(i)->setAdaptiveSampling(false);
        }
        plot->replot(QCustomPlot::rpRefreshHint);
    }

    const bool ok = plot->savePng(f, 0, 0, 1.5);

    if (plot == m_plotCurve) {
        for (int i = 0; i < plot->graphCount(); ++i) {
            plot->graph(i)->setAdaptiveSampling(adaptiveBackup[i]);
        }
        plot->replot(QCustomPlot::rpQueuedReplot);
    }

    if (!ok) {
        QMessageBox::warning(this, tr("导出 PNG"), tr("写入失败：%1").arg(f));
    }
}

void PlotDialog::onExportCsv()
{
    QCustomPlot* plot = activePlot();
    if (plot == m_plotHeat && m_hasHeatmap && !m_heatZ.isEmpty()) {
        const QString f = QFileDialog::getSaveFileName(
            this, tr("导出 CSV"), QString(), tr("CSV 文件 (*.csv)"));
        if (f.isEmpty())
            return;
        QFile fp(f);
        if (!fp.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("导出 CSV"), tr("无法写入：%1").arg(f));
            return;
        }
        QTextStream ts(&fp);
        ts << "key";
        for (int c = 0; c < m_heatCols; ++c) {
            ts << ',' << m_heatX[c];
        }
        ts << '\n';
        for (int r = 0; r < m_heatRows; ++r) {
            ts << m_heatY[r];
            for (int c = 0; c < m_heatCols; ++c) {
                ts << ',' << m_heatZ[r * m_heatCols + c];
            }
            ts << '\n';
        }
        return;
    }

    if (m_x.isEmpty() || m_ys.isEmpty())
        return;
    const QString f = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), QString(), tr("CSV 文件 (*.csv)"));
    if (f.isEmpty())
        return;
    QFile fp(f);
    if (!fp.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出 CSV"), tr("无法写入：%1").arg(f));
        return;
    }
    QTextStream ts(&fp);
    ts << "x";
    for (int c = 0; c < m_ys.size(); ++c) {
        ts << ','
           << (c < m_names.size() ? m_names[c] : QStringLiteral("y%1").arg(c + 1));
    }
    ts << '\n';
    for (int i = 0; i < m_x.size(); ++i) {
        ts << m_x[i];
        for (int c = 0; c < m_ys.size(); ++c) {
            ts << ',' << (i < m_ys[c].size() ? m_ys[c][i] : 0.0);
        }
        ts << '\n';
    }
}
