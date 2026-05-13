# 定义数据处理的信号，gui通过槽函数更新ui
# 创建一个接口库，仅用于收集头文件路径
add_library(core_dao_interfaces INTERFACE
)

# 需要依赖的其他库
target_link_libraries(core_dao_interfaces INTERFACE
    core_interfaces
    Qt::Core
)

target_include_directories(core_dao_interfaces INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
