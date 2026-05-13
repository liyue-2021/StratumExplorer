#pragma once
#include <QString>

/**
 * @brief 全局 QRC 资源路径常量
 * @note 所有 qrc 资源前缀统一在此维护，禁止硬编码
 */
struct ResourceConst
{
    // 翻译文件（i18n）
    inline static const QString TRANSLATIONS = ":/translations/";
    // 字体文件
    inline static const QString FONT         = ":/font/";
    // 图标文件
    inline static const QString ICONS        = ":/icons/";
    // 图片资源
    inline static const QString IMAGES       = ":/images/";
    // 样式表(qss)
    inline static const QString STYLES       = ":/styles/";
};

/**
 * @brief 后续可扩展其他全局常量
 * 例：系统配置、通用数值、默认参数等
 */
struct AppConst
{
    inline static const QString DEFAULT_LANGUAGE = "zh_CN";
};
