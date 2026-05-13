#ifndef IGUICONTEXT_H
#define IGUICONTEXT_H

#include <QSharedPointer>

// 前向声明接口依赖（无具体头文件依赖，极致解耦）
class ICoreContext;
class ILicenseController;
class IRealtimeController;

/**
 * @brief GUI上下文抽象接口
 * 定义GUI模块核心访问方法，依赖倒置：高层模块依赖接口而非实现
 */
class IGuiContext
{
public:
    // 接口必须定义虚析构函数，保证多态安全释放
    virtual ~IGuiContext() = default;

    // ===================== 纯虚接口方法 =====================
    /// 初始化GUI上下文
    virtual void init(ICoreContext& coreContext) = 0;
    /// 获取初始化状态
    virtual bool isInitialized() const = 0;
    /// 获取许可证控制器（核心业务接口）
    virtual QSharedPointer<ILicenseController> getLicenseController() const = 0;
};

#endif // IGUICONTEXT_H
