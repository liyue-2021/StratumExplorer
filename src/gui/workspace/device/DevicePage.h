#ifndef DEVICE_Page_H
#define DEVICE_Page_H

// 替换为基类头文件（已封装QWidget+IDataOperateInterface）
#include "widgets/BaseDataOperateWidget.h"

namespace Ui {
    class DevicePage;
}

// 继承基类，无需重复继承QWidget和接口
class DevicePage : public BaseDataOperateWidget
{
    Q_OBJECT

public:
    explicit DevicePage(QWidget *parent = nullptr);
    ~DevicePage();

protected:
    // 重写基类纯虚函数
    void clearSelfDataAndDisableBtn() override;
    void loadSelfDataAndEnableBtn() override;

private:
    Ui::DevicePage *ui;
};

#endif // DEVICE_Page_H
