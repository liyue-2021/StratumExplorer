#对外暴露的接口
add_subdirectory(context)
add_subdirectory(dao)
add_subdirectory(models)
add_subdirectory(service)
add_subdirectory(utils)

# 创建一个接口库，仅用于收集头文件路径
add_library(core_interfaces INTERFACE
)

# 将当前目录（即 interfaces 目录）添加为 INTERFACE 包含路径
target_include_directories(core_interfaces INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 需要依赖的其他库
target_link_libraries(core_interfaces INTERFACE
    core_interfaces_context
    core_interfaces_dao
    core_interfaces_models
    core_interfaces_service
    core_interfaces_utils
)
