# 数据处理页面
# 收集源文件和头文件
set(SOURCES
    ProcessingPage.cpp
)
set(HEADERS
    ProcessingPage.h
)
set(FORMS
    ProcessingPage.ui
)

# 工作流编辑器子模块
add_subdirectory(workflow)

# 创建对象库
add_library(gui_workspace_processing STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# 指定需要链接的其他库
target_link_libraries(gui_workspace_processing PUBLIC
    gui_workspace_processing_workflow
)

target_link_libraries(gui_workspace_processing PRIVATE
    GuiDepends
    core
    gui_utils
    gui_widgets
    gui_interfaces
    gui_controller
)
