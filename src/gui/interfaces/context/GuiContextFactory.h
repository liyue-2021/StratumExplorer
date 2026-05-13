#ifndef GUICONTEXTFACTORY_H
#define GUICONTEXTFACTORY_H

// 仅依赖抽象接口，无具体实现类依赖
#include "IGuiContext.h"

/**
 * @brief 上下文工厂
 * 封装所有上下文实例的创建逻辑，统一入口、解耦创建与使用
 */
class GuiContextFactory
{
public:
    // 禁止实例化工厂类（纯静态工具类）
    GuiContextFactory() = delete;
    ~GuiContextFactory() = delete;
    GuiContextFactory(const GuiContextFactory&) = delete;
    GuiContextFactory& operator=(const GuiContextFactory&) = delete;

    // ===================== 核心工厂方法 =====================
    /// 获取 GUI 上下文实例（返回抽象接口，隐藏具体实现）
    static IGuiContext& getGuiContext();
};

#endif // GUICONTEXTFACTORY_H
