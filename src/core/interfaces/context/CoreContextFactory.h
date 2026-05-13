#ifndef CoreContextFactory_H
#define CoreContextFactory_H

#include <memory>
// 仅依赖接口，不依赖实现类
#include "ICoreContext.h"

/**
 * @brief 上下文工厂类
 * @details 统一封装所有上下文实例的获取入口，解耦创建与使用
 *          遵循：接口导向、无状态、静态工厂
 */
class CoreContextFactory
{
public:
    // 禁止实例化工厂类（纯静态工具类）
    CoreContextFactory() = delete;
    CoreContextFactory(const CoreContextFactory&) = delete;
    CoreContextFactory& operator=(const CoreContextFactory&) = delete;

    // ==============================================
    // 🔥 核心工厂方法：返回【抽象接口】
    // ==============================================
    /**
     * @brief 获取核心上下文实例（接口导向）
     * @return ICoreContext& 接口引用
     */
    static ICoreContext& getCoreContext();
};

#endif // CoreContextFactory_H
