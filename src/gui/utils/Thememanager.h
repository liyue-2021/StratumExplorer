#pragma once

#include <QObject>
#include <QPalette>
#include <QString>

namespace gui::utils {

/**
 * @brief 主题类型枚举
 */
enum class Theme {
    Light,
    Dark,
    System  // 跟随系统
};

/**
 * @brief 应用程序主题管理器（单例）
 *
 * 负责管理全局主题、调色板、样式表，并在主题切换时通知所有组件。
 * 属于纯 GUI 工具，不依赖 core 层。
 */
class ThemeManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ThemeManager)

public:
    // 获取单例实例
    static ThemeManager* instance();

    // 获取当前主题
    Theme currentTheme() const { return m_currentTheme; }

    // 设置主题（切换时自动应用调色板并发射信号）
    void setTheme(Theme theme);

    // 便捷方法：切换亮色/暗色（忽略 System）
    void toggleTheme();

    // 获取特定主题的调色板（用于预览或自定义控件）
    static QPalette paletteForTheme(Theme theme);

signals:
    // 主题变更信号，参数为新主题
    void themeChanged(gui::utils::Theme newTheme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    // 应用调色板到 QApplication
    void applyPalette(const QPalette& palette);

private:
    Theme m_currentTheme = Theme::Light;
};

} // namespace gui::utils
