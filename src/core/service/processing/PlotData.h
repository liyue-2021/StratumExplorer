#ifndef PLOTDATA_H
#define PLOTDATA_H

#include <QObject>
#include <QString>

#include "service/processing/IPlotData.h"

class PlotData : public QObject, public IPlotData
{
    Q_OBJECT

public:
    explicit PlotData(QObject* parent = nullptr);
    ~PlotData() override;

    void LoadPlotData(const PlotRequest& req, PlotDisplayData* out, QString* error) override;
};

#endif // PLOTDATA_H
