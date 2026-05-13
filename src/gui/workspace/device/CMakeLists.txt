# 设备管理页面
# 收集源文件和头文件
set(SOURCES
    DevicePage.cpp
)
set(HEADERS
    DevicePage.h
)
set(FORMS
    DevicePage.ui
)

# 创建对象库
add_library(gui_workspace_device STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# 指定需要链接的其他库
target_link_libraries(gui_workspace_device PRIVATE
    GuiDepends
    core
    gui_utils
    gui_widgets
    gui_interfaces
    gui_controller
)
