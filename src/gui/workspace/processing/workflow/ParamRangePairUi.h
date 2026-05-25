#ifndef PARAM_RANGE_PAIR_UI_H
#define PARAM_RANGE_PAIR_UI_H

#include "ParamRangePair.h"

#include <QFormLayout>
#include <functional>

class QWidget;

namespace processing::gui
{

    void addParamRangePairRow(QFormLayout *form,
                              QWidget *panel,
                              const ParamRangePairSpec &spec,
                              std::function<QVariantMap()> fetchParams,
                              std::function<void(QVariantMap &)> applyParams);

} // namespace processing::gui

#endif // PARAM_RANGE_PAIR_UI_H
