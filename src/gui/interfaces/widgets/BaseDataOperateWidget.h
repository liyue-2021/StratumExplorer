#ifndef BASEDATAOPERATEWIDGET_H
#define BASEDATAOPERATEWIDGET_H

#include <QWidget>
#include "IDataOperateInterface.h"
#include "DataOperateTools.h"
#include <QList>

class BaseDataOperateWidget : public QWidget, public IDataOperateInterface
{
    Q_OBJECT
public:
    explicit BaseDataOperateWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    // ========================
    // 基类最终实现：统一处理
    // 1. 清空自身数据
    // 2. 清空所有子控件
    // ========================
public slots:
    void clearAllDataAndDisableBtn() final {
        // 1. 子类实现：清空自己的UI数据
        clearSelfDataAndDisableBtn();
        // 2. 基类处理：批量清空所有子接口
        DataOperateTools::batchClear(m_children);
    }

    void loadAllDataAndEnableBtn() final {
        // 1. 子类实现：加载自己的UI数据
        loadSelfDataAndEnableBtn();
        // 2. 基类处理：批量加载所有子接口
        DataOperateTools::batchLoad(m_children);
    }

protected:
    // ========================
    // 子类必须重写：自身数据逻辑
    // ========================
    virtual void clearSelfDataAndDisableBtn() = 0;   // 子类：清空自己的按钮/输入框等
    virtual void loadSelfDataAndEnableBtn() = 0;    // 子类：加载自己的数据

    // ========================
    // 子类调用：添加子接口
    // ========================
    void addChild(IDataOperateInterface* child) {
        if (child) m_children.append(child);
    }

private:
    // 基类私有：子接口列表（子类无需管理）
    QList<IDataOperateInterface*> m_children;
};

#endif // BASEDATAOPERATEWIDGET_H
