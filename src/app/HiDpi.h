#pragma once

#include <QCoreApplication>
#include <QGuiApplication>

/** 在构造 QApplication 之前调用，配置高 DPI 与图标缩放。 */
inline void setupHiDpiBeforeQApplication()
{
    // Qt 6 默认已启用高 DPI 缩放，无需 AA_EnableHighDpiScaling（Qt5 专用，Qt6 已移除）
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}
