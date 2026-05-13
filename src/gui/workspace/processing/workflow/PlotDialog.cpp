#include "PlotDialog.h"

#include "qcustomplot.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtMath>
#include <random>

using namespace processing::gui;

namespace {
// 一组配色，循环使用
const QVector<QColor> kPalette = {
    QColor("#1976D2"), QColor("#E64A19"), QColor("#388E3C"),
    QColor("#7B1FA2"), QColor("#F57C00"), QColor("#0097A7"),
};
} // namespace

PlotDialog::PlotDialog(const QString& title, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    resize(900, 560);

    m_plot = new QCustomPlot(this);
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                            QCP::iSelectAxes | QCP::iSelectLegend |
                            QCP::iSelectPlottables);
    m_plot->legend->setVisible(true);
    m_plot->legend->setBrush(QColor(255, 255, 255, 200));
    m_plot->xAxis->setLabel(tr("样本 / 时间"));
    m_plot->yAxis->setLabel(tr("幅值"));
    m_plot->axisRect()->setupFullAxesBox(true);

    m_btnPng = new QPushButton(tr("导出 PNG…"), this);
    m_btnCsv = new QPushButton(tr("导出 CSV…"), this);
    auto* btnClose = new QPushButton(tr("关闭"), this);

    connect(m_btnPng,  &QPushButton::clicked, this, &PlotDialog::onExportPng);
    connect(m_btnCsv,  &QPushButton::clicked, this, &PlotDialog::onExportCsv);
    connect(btnClose,  &QPushButton::clicked, this, &QDialog::accept);

    auto* btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(m_btnPng);
    btns->addWidget(m_btnCsv);
    btns->addWidget(btnClose);

    auto* root = new QVBoxLayout(this);
    m_metaLabel = new QLabel(this);
    m_metaLabel->setStyleSheet("background:#ECEFF1; color:#37474F; padding:6px;");
    m_metaLabel->setWordWrap(true);
    m_metaLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_metaLabel->hide();   // setMetaInfo 会遵循可见
    root->addWidget(m_metaLabel);
    root->addWidget(m_plot, 1);
    root->addLayout(btns);
}

void PlotDialog::setMetaInfo(const QStringList& outputFiles, const QString& summary) {
    QStringList lines;
    if (!summary.isEmpty()) lines << summary;
    if (outputFiles.isEmpty()) {
        lines << tr("该节点未产生输出文件。");
    } else {
        for (const auto& f : outputFiles) {
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
    m_metaLabel->setText(lines.join("\n"));
    m_metaLabel->setVisible(true);
}

void PlotDialog::setSeries(const QVector<double>& x,
                           const QVector<QVector<double>>& ys,
                           const QStringList& names) {
    m_x     = x;
    m_ys    = ys;
    m_names = names;

    m_plot->clearGraphs();
    double yMin =  std::numeric_limits<double>::infinity();
    double yMax = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < ys.size(); ++i) {
        auto* g = m_plot->addGraph();
        QPen pen(kPalette[i % kPalette.size()]);
        pen.setWidthF(1.6);
        g->setPen(pen);
        g->setName(i < names.size() ? names[i] : tr("曲线 %1").arg(i + 1));
        g->setData(x, ys[i], /*alreadySorted=*/true);

        for (double v : ys[i]) {
            yMin = std::min(yMin, v);
            yMax = std::max(yMax, v);
        }
    }
    if (!x.isEmpty()) m_plot->xAxis->setRange(x.first(), x.last());
    if (yMin < yMax) {
        const double pad = (yMax - yMin) * 0.08;
        m_plot->yAxis->setRange(yMin - pad, yMax + pad);
    }
    m_plot->replot();
}

void PlotDialog::setDemoFromSeed(const QString& seed, int curveCount) {
    constexpr int N = 512;
    QVector<double> x(N);
    for (int i = 0; i < N; ++i) x[i] = i;

    QVector<QVector<double>> ys;
    QStringList names;

    // 用 seed 的 hash 作为种子，保证同一节点每次重开都是同一条曲线
    std::mt19937 rng(static_cast<uint32_t>(qHash(seed)));
    std::uniform_real_distribution<double> noise(-0.15, 0.15);

    for (int c = 0; c < curveCount; ++c) {
        QVector<double> y(N);
        const double f1 = 0.04 + (rng() % 100) / 5000.0;
        const double f2 = 0.13 + (rng() % 100) / 4000.0;
        const double ph = (rng() % 628) / 100.0;
        for (int i = 0; i < N; ++i) {
            y[i] = std::sin(f1 * i + ph) * 1.0
                 + std::sin(f2 * i) * 0.4
                 + noise(rng);
        }
        ys.push_back(std::move(y));
        names << tr("通道 %1").arg(c + 1);
    }
    setSeries(x, ys, names);
}

void PlotDialog::onExportPng() {
    const QString f = QFileDialog::getSaveFileName(
        this, tr("导出 PNG"), QString(),
        tr("PNG 图像 (*.png)"));
    if (f.isEmpty()) return;
    if (!m_plot->savePng(f, 0, 0, 1.5)) {
        QMessageBox::warning(this, tr("导出 PNG"),
                             tr("写入失败：%1").arg(f));
    }
}

void PlotDialog::onExportCsv() {
    if (m_x.isEmpty() || m_ys.isEmpty()) return;
    const QString f = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), QString(),
        tr("CSV 文件 (*.csv)"));
    if (f.isEmpty()) return;
    QFile fp(f);
    if (!fp.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出 CSV"),
                             tr("无法写入：%1").arg(f));
        return;
    }
    QTextStream ts(&fp);
    ts << "x";
    for (int c = 0; c < m_ys.size(); ++c) {
        ts << "," << (c < m_names.size() ? m_names[c] : QStringLiteral("y%1").arg(c + 1));
    }
    ts << "\n";
    for (int i = 0; i < m_x.size(); ++i) {
        ts << m_x[i];
        for (int c = 0; c < m_ys.size(); ++c) {
            ts << "," << (i < m_ys[c].size() ? m_ys[c][i] : 0.0);
        }
        ts << "\n";
    }
}
