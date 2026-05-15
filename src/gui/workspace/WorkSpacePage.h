#ifndef WORKSPACE_Page_H
#define WORKSPACE_Page_H

#include "widgets/BaseDataOperateWidget.h"
// 保留原有UI头文件
#include <QWidget>

namespace Ui {
    class WorkSpacePage;
}

class WorkSpacePage : public BaseDataOperateWidget
{
    Q_OBJECT

public:
    explicit WorkSpacePage(QWidget *parent = nullptr);
    ~WorkSpacePage();

protected:
    void clearSelfDataAndDisableBtn() override;  // 清空自身UI（本页面无自身数据，空实现）
    void loadSelfDataAndEnableBtn() override;   // 加载自身数据（本页面无自身数据，空实现）

public:
    bool promptSaveCurrentWorkflowIfNeeded();

private:
    Ui::WorkSpacePage *ui;
    void addTabPage(QWidget* page, const QString& tabName);
};

#endif // WORKSPACE_Page_H
