#ifndef ICORE_CONTEXT_H
#define ICORE_CONTEXT_H

#include <memory>
// 仅依赖Service接口，无DAO、数据库依赖
#include "service/system/ILicenseService.h"

// 纯虚核心上下文接口：只暴露Service获取方法
class ICoreContext
{
public:
    // 虚析构函数：保证子类安全析构
    virtual ~ICoreContext() = default;

    // ============= 纯虚接口：仅定义Service获取方法 =============
    virtual std::shared_ptr<ILicenseService> getLicenseService() const = 0;

};

#endif // ICORE_CONTEXT_H
