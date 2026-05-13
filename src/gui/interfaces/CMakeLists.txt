# 对外暴露的gui服务
add_subdirectory(context)
add_subdirectory(controller)
add_subdirectory(guiservice)
add_subdirectory(widgets)

# 创建一个接口库，仅用于收集头文件路径
add_library(gui_interfaces INTERFACE
)

# 将当前目录（即 interfaces 目录）添加为 INTERFACE 包含路径
target_include_directories(gui_interfaces INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 需要依赖的其他库
target_link_libraries(gui_interfaces INTERFACE
        gui_interfaces_context
        gui_interfaces_controller
        gui_interfaces_guiservice
        gui_interfaces_widgets
)
