#ifndef DATAOPERATETOOLS_H
#define DATAOPERATETOOLS_H

#include "IDataOperateInterface.h"
#include <QList>

class DataOperateTools
{
public:
    // 批量清空：遍历所有子接口执行清空
    static void batchClear(const QList<IDataOperateInterface*>& children)
    {
        for (auto* item : children) {
            if (item) item->clearAllDataAndDisableBtn();
        }
    }

    // 批量加载：遍历所有子接口执行加载
    static void batchLoad(const QList<IDataOperateInterface*>& children)
    {
        for (auto* item : children) {
            if (item) item->loadAllDataAndEnableBtn();
        }
    }
};

#endif // DATAOPERATETOOLS_H
