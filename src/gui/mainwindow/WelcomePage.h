#ifndef WELCOME_Page_H
#define WELCOME_Page_H

// 替换为基类头文件（已封装 QWidget + IDataOperateInterface）
#include "widgets/BaseDataOperateWidget.h"

namespace Ui {
    class WelcomePage;
}

// 统一继承基类，无需重复多继承
class WelcomePage : public BaseDataOperateWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);
    ~WelcomePage();

protected:
    // 重写基类纯虚函数（自身数据处理）
    void clearSelfDataAndDisableBtn() override;
    void loadSelfDataAndEnableBtn() override;

private:
    Ui::WelcomePage *ui;
};

#endif // WELCOME_Page_H
