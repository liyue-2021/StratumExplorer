#ifndef STRATUMEXPLORER_WELLPAGE_H
#define STRATUMEXPLORER_WELLPAGE_H

// 替换：直接依赖封装好的基类（基类已包含QWidget+IDataOperateInterface）
#include "widgets/BaseDataOperateWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class WellPage; }
QT_END_NAMESPACE

// 关键修改：单继承 BaseDataOperateWidget（基类已实现双重继承+子控件管理）
class WellPage : public BaseDataOperateWidget
{
    Q_OBJECT

public:
    explicit WellPage(QWidget* parent = nullptr);
    ~WellPage() override;

protected:
    // 必须重写：基类纯虚函数 → 实现自身UI数据逻辑
    void clearSelfDataAndDisableBtn() override;
    void loadSelfDataAndEnableBtn() override;

private:
    Ui::WellPage* ui;
};

#endif //STRATUMEXPLORER_WELLPAGE_H
