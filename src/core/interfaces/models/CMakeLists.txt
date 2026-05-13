# 创建一个接口库，仅用于收集头文件路径
file(GLOB_RECURSE DATA_SOURCES "*.cpp")
file(GLOB_RECURSE DATA_HEADERS "*.h")
add_library(core_interfaces_models INTERFACE
        constants/Constants.h
    po/IUser.h
    ${DATA_SOURCES}
    ${DATA_HEADERS}
)

# 子模块（工作流相关数据模型）
add_subdirectory(processing)

# 需要依赖的其他库
target_link_libraries(core_interfaces_models INTERFACE
    Qt::Core
    core_interfaces_models_processing
)

target_include_directories(core_interfaces_models INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
