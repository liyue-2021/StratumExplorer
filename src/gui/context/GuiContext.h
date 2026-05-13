#ifndef GUICONTEXT_H
#define GUICONTEXT_H

#include <QSharedPointer>
#include <QThread>
#include "context/IGuiContext.h"

// 前向声明（保持不变，接口已提前声明）
class ICoreContext;
class ILicenseController;

// 核心修改：继承IGuiContext抽象接口
class GuiContext : public IGuiContext
{
public:
    // 禁止拷贝/移动（保持不变）
    GuiContext(const GuiContext&) = delete;
    GuiContext& operator=(const GuiContext&) = delete;
    GuiContext(GuiContext&&) = delete;
    GuiContext& operator=(GuiContext&&) = delete;

    friend class GuiContextFactory;

    // ===================== 实现接口纯虚方法 =====================
    // 方法签名与接口完全一致
    void init(ICoreContext& coreContext) override;  // override 关键字标记重写
    bool isInitialized() const override;
    QSharedPointer<ILicenseController> getLicenseController() const override;

private:
    GuiContext();
    ~GuiContext() override;  // 虚析构，匹配接口

    bool m_isInitialized = false;
    QThread* m_backgroundThread = nullptr;
    QSharedPointer<ILicenseController> m_licenseController;
};

#endif // GUICONTEXT_H
