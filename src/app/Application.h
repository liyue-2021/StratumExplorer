#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class MainWindow;

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    ~Application();

    void run();  // 显示主窗口

private:
    MainWindow *m_mainWindow;  // 直接用指针管理
};

#endif // APPLICATION_H
