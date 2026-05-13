# 创建一个qt静态库，仅用于收集头文件路径
add_library(core_interfaces_service_base INTERFACE
        system/ILicenseService.h
        system/IUserService.h
)
# 需要依赖的其他库
target_link_libraries(core_interfaces_service_base INTERFACE
        Qt::Core
        core_interfaces_models
        core_interfaces_dao
)

# 创建一个子接口库，仅用于收集头文件路径
add_library(core_interfaces_service_qt STATIC
)
# 需要依赖的其他库
target_link_libraries(core_interfaces_service_qt PUBLIC
        Qt::Core
        core_interfaces_models
        core_interfaces_dao
)


# 创建一个接口库，仅用于收集头文件路径
add_library(core_interfaces_service INTERFACE
        core_interfaces_service_qt
        core_interfaces_service_base
)

# 子模块（工作流相关服务接口）
add_subdirectory(processing)

target_link_libraries(core_interfaces_service INTERFACE
        core_interfaces_service_processing
)

target_include_directories(core_interfaces_service INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
