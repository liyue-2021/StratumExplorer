#ifndef IDATAOPERATEINTERFACE_H
#define IDATAOPERATEINTERFACE_H

// Qt 接口类：纯虚函数的抽象类
class IDataOperateInterface
{
public:
    // 析构函数（虚函数，保证多态释放）
    virtual ~IDataOperateInterface() = default;

    // 1. 清理所有页面数据 + 禁用交换按钮（纯虚函数）
    virtual void clearAllDataAndDisableBtn() = 0;

    // 2. 加载所有数据 + 启用交换按钮（纯虚函数）
    virtual void loadAllDataAndEnableBtn() = 0;
};

// Qt 元对象系统声明接口（可选，用于多态/插件扩展）
// #define IDATAOPERATEINTERFACE_IID "com.example.IDataOperateInterface"
// Q_DECLARE_INTERFACE(IDataOperateInterface, IDATAOPERATEINTERFACE_IID)

#endif // IDATAOPERATEINTERFACE_H
