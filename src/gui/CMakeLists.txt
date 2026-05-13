# 定义静态库 gui，用于封装 gui 相关功能
add_library(gui STATIC
)

# add_subdirectory(adapter)
add_subdirectory(context)
add_subdirectory(controller)
add_subdirectory(depends)
add_subdirectory(guiservice)
add_subdirectory(interfaces)
add_subdirectory(mainwindow)
add_subdirectory(utils)
add_subdirectory(widgets)
add_subdirectory(workspace)

# 设置 gui 库的头包含目录
target_include_directories(gui PUBLIC
    # TODO mainwindow的定义移动到guiservice目录下
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow
)

target_include_directories(gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 指定 gui 库需要链接的其他库
target_link_libraries(gui PUBLIC
    GuiDepends
    gui_interfaces
    core
)

target_link_libraries(gui PRIVATE
    # gui_adapter
    gui_context
    gui_controller
    gui_guiservice
    gui_mainwindow
    gui_utils
    gui_widgets
    gui_workspace
)
