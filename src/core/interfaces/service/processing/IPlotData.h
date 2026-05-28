#ifndef IPLOTDATA_H
#define IPLOTDATA_H

#include <QString>
#include <vector>

struct PlotRequest
{
    QString h5Path;     // h5 数据文件绝对路径
    QString kind;       // 图表类型：Overview / Channel / Window
    int     channelIndex = 0; // 通道编号，Channel 时使用
    int     t_start = 0;      // 起始采样点，Window 时使用
    int     t_end = 0;        // 结束采样点，Window 时使用
    int     ch_start = 0;     // 起始通道，Window 时使用
    int     ch_end = 0;       // 结束通道，Window 时使用
};

struct PlotDisplayData
{
    std::vector<float> data;

    int rows = 0; // Y 轴方向数量（通道）
    int cols = 0; // X 轴方向数量（采样点）

    std::vector<int> x;
    std::vector<int> y;

    void clear()
    {
        data.clear();
        x.clear();
        y.clear();
        rows = 0;
        cols = 0;
    }
};

class IPlotData
{
public:
    virtual ~IPlotData() = default;

    virtual void LoadPlotData(const PlotRequest& req, PlotDisplayData* out, QString* error) = 0;
};

#endif // IPLOTDATA_H
