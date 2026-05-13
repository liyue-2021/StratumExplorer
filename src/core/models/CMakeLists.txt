# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)

# 创建对象库
add_library(core_models STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_models PRIVATE
    CoreDepends
    core_interfaces
)
