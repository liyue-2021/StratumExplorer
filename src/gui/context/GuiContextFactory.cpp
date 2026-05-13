#include "context/GuiContextFactory.h"
// 工厂内部仅此处依赖具体实现类，外部完全解耦
#include "GuiContext.h"

// 工厂方法：返回抽象接口，实际返回单例实例
IGuiContext& GuiContextFactory::getGuiContext()
{
    // 🔥 工厂接管单例（C++11线程安全，保留原有注释）
    static GuiContext ctx;
    return ctx;
}
