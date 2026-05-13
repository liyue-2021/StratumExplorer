//TODO 增加异常捕获
#include <QApplication>
#include "Application.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_UseOpenGLES);
    // QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    // QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QApplication a(argc, argv);

    Application application;
    // 创建并显示主窗口
    application.run();

    a.exec();
    return 0;
}
