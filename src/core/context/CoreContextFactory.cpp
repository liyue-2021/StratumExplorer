#include "context/CoreContextFactory.h"
#include "CoreContext.h"

// 获取核心上下文：工厂内部关联实现类，对外只暴露接口
ICoreContext& CoreContextFactory::getCoreContext()
{
    // 🔥 工厂管理单例（C++11线程安全，保留原有注释）
    static CoreContext ctx;
    return ctx;
}
