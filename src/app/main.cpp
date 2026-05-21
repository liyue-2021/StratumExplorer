//TODO 增加异常捕获
#include <QApplication>

#include "Application.h"
#include "HiDpi.h"

int main(int argc, char *argv[])
{
    setupHiDpiBeforeQApplication();

    QApplication::setAttribute(Qt::AA_UseOpenGLES);

    QApplication a(argc, argv);

    Application application;
    // 创建并显示主窗口
    application.run();

    a.exec();
    return 0;
}
