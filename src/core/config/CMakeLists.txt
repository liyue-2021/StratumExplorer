#全局配置相关代码，语言等
# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)

# 创建对象库
add_library(core_config STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_config PRIVATE
    CoreDepends
    core_interfaces
)

target_include_directories(core_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
