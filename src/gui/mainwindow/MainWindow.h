#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "WelcomePage.h"
// 替换：主窗口专用数据操作基类（已封装 QMainWindow + IDataOperateInterface）
#include "widgets/BaseDataOperateMainWindow.h"
//TODO 为啥需要相对路径，去掉..找不到头文件
#include "../workspace/WorkSpacePage.h"

namespace Ui {
    class MainWindow;
}

// 修改：继承基类 BaseDataOperateMainWindow
class MainWindow : public BaseDataOperateMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    //TODO 开发临时使用
    void togglePages();
    WelcomePage *welcomePage;
    WorkSpacePage *workspacePage;
    void addStackedPage(QWidget* page);

protected:
    // 重写基类纯虚函数（统一规范）
    void clearSelfDataAndDisableBtn() override;
    void loadSelfDataAndEnableBtn() override;
};

#endif // MAINWINDOW_H
