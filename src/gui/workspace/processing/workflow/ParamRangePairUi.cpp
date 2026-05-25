#include "ParamRangePairUi.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QtMath>

namespace processing::gui
{

namespace
{

double upperLimitForSpec(const QVariantMap &params, const ParamRangePairSpec &spec)
{
    if (spec.maxLimitFsHalf)
    {
        const double fs = qMax(1e-9, params.value(QStringLiteral("fs")).toDouble());
        return fs / 2.0;
    }
    return 1e12;
}

} // namespace

void addParamRangePairRow(QFormLayout *form,
                          QWidget *panel,
                          const ParamRangePairSpec &spec,
                          std::function<QVariantMap()> fetchParams,
                          std::function<void(QVariantMap &)> applyParams)
{
    auto *row = new QWidget(panel);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8);

    auto *spMin = new QDoubleSpinBox(row);
    auto *sep = new QLabel(QStringLiteral("~"), row);
    auto *spMax = new QDoubleSpinBox(row);
    sep->setAlignment(Qt::AlignCenter);
    sep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    spMin->setDecimals(3);
    spMax->setDecimals(3);

    const QVariantMap p0 = fetchParams();
    const double upper0 = upperLimitForSpec(p0, spec);
    double vMin = p0.value(spec.minKey, spec.defaultMin).toDouble();
    double vMax = p0.value(spec.maxKey, spec.defaultMax).toDouble();
    const double lower0 = spec.minExclusive ? spec.defaultMin + 1e-9 : spec.defaultMin;
    vMin = qBound(lower0, vMin, upper0);
    vMax = qBound(vMin + 1e-6, vMax, upper0);
    spMin->setRange(lower0, qMax(lower0, vMax - 1e-6));
    spMax->setRange(vMin + 1e-6, upper0);
    spMin->setValue(vMin);
    spMax->setValue(vMax);

    auto syncLimits = [spMin, spMax, fetchParams, spec]()
    {
        const QVariantMap p = fetchParams();
        const double upper = upperLimitForSpec(p, spec);
        double curMin = spMin->value();
        double curMax = spMax->value();
        spMin->blockSignals(true);
        spMax->blockSignals(true);
        const double lower = spec.minExclusive ? spec.defaultMin + 1e-9 : spec.defaultMin;
        spMin->setRange(lower, qMax(lower, curMax - 1e-6));
        spMax->setRange(curMin + 1e-6, upper);
        spMin->setValue(qBound(spMin->minimum(), curMin, spMin->maximum()));
        spMax->setValue(qBound(spMax->minimum(), curMax, spMax->maximum()));
        spMin->blockSignals(false);
        spMax->blockSignals(false);
    };

    // 必须按值捕获 std::function：调用方 rebuildParamForm 返回后栈上的形参引用会失效
    auto commit = [applyParams, fetchParams, spec, spMin, spMax, syncLimits]()
    {
        syncLimits();
        QVariantMap p = fetchParams();
        p.insert(spec.minKey, spMin->value());
        p.insert(spec.maxKey, spMax->value());
        applyParams(p);
    };

    QObject::connect(spMin, qOverload<double>(&QDoubleSpinBox::valueChanged), panel,
                     [commit, syncLimits](double) { syncLimits(); commit(); });
    QObject::connect(spMax, qOverload<double>(&QDoubleSpinBox::valueChanged), panel,
                     [commit, syncLimits](double) { syncLimits(); commit(); });

    h->setSpacing(6);
    h->addWidget(spMin, 1);
    h->addWidget(sep);
    h->addWidget(spMax, 1);
    auto *lbl = new QLabel(spec.rowLabel, panel);
    lbl->setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
    lbl->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    form->addRow(lbl, row);
}

} // namespace processing::gui
