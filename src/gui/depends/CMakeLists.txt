# gui 专属依赖
add_library(GuiDepends INTERFACE)
target_link_libraries(GuiDepends INTERFACE
    GlobalDepends
    Qt::Core
    Qt::Gui
    Qt::Charts
    Qt::Widgets
    Qt::Qml
    Qt::Quick
)
