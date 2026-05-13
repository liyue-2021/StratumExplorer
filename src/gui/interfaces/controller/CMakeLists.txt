# 创建一个接口库，仅用于收集头文件路径
add_library(gui_interfaces_controller STATIC
    system/ILicenseController.h
)

# 需要依赖的其他库
target_link_libraries(gui_interfaces_controller PRIVATE
    Qt::Core
)

target_include_directories(gui_interfaces_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
