#include "ThemeManager.h"
#include <QApplication>
#include <QStyleHints>

namespace gui::utils {

// 静态实例
static ThemeManager* s_instance = nullptr;

ThemeManager* ThemeManager::instance()
{
    if (!s_instance) {
        s_instance = new ThemeManager(qApp);
    }
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    // 初始主题：跟随系统（如果 Qt6 支持系统主题）
    // 这里简单起见设为亮色，也可以从 QSettings 读取上次保存的主题
    setTheme(Theme::Light);
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme == theme)
        return;

    m_currentTheme = theme;
    applyPalette(paletteForTheme(theme));
    emit themeChanged(theme);
}

void ThemeManager::toggleTheme()
{
    if (m_currentTheme == Theme::Light)
        setTheme(Theme::Dark);
    else if (m_currentTheme == Theme::Dark)
        setTheme(Theme::Light);
    // System 模式下也可处理，但为简化，直接切换亮暗
}

QPalette ThemeManager::paletteForTheme(Theme theme)
{
    QPalette palette;

    // 亮色主题 (默认)
    if (theme == Theme::Light) {
        palette.setColor(QPalette::Window, Qt::white);
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, QColor(240, 240, 240));
        palette.setColor(QPalette::AlternateBase, QColor(220, 220, 220));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(220, 220, 220));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }
    // 暗色主题
    else if (theme == Theme::Dark) {
        palette.setColor(QPalette::Window, QColor(43, 43, 43));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(30, 30, 30));
        palette.setColor(QPalette::AlternateBase, QColor(50, 50, 50));
        palette.setColor(QPalette::ToolTipBase, QColor(30, 30, 30));
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }
    // System: 这里简单起见，返回当前系统调色板（Qt6 可通过 QStyleHints 获取系统主题）
    // 实际可调用 QApplication::styleHints()->colorScheme() 判断
    else if (theme == Theme::System) {
        // 示例：如果系统为暗色模式则返回暗色调色板
        // 但为了简洁，此处直接返回亮色主题
        return paletteForTheme(Theme::Light);
    }

    return palette;
}

void ThemeManager::applyPalette(const QPalette& palette)
{
    qApp->setPalette(palette);
    // 可选：更新样式表，确保所有控件重新计算样式
    qApp->setStyleSheet(""); // 强制刷新
}

} // namespace gui::utils
