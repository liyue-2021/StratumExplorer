# 菜单按钮定义
# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)
set(FORMS
)

# 创建对象库
add_library(gui_mainwindow_menus STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# 指定 gui_mainwindow 库需要链接的其他库
target_link_libraries(gui_mainwindow_menus PRIVATE
    GuiDepends
    core
    gui_mainwindow_dialogs
    gui_interfaces
    gui_controller
)
