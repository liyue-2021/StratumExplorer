#对外暴露的工具类
# 创建一个接口库，仅用于收集头文件路径
add_library(core_interfaces_utils INTERFACE

)

# 需要依赖的其他库
target_link_libraries(core_interfaces_utils INTERFACE

)

target_include_directories(core_interfaces_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
