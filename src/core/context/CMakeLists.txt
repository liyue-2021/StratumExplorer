#全局配置相关代码，语言等
# 收集源文件和头文件
set(SOURCES
    CoreContext.cpp
    CoreContextFactory.cpp
)
set(HEADERS
    CoreContext.h
)

# 创建对象库
add_library(core_context STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_context PRIVATE
    CoreDepends
    core_interfaces
)

target_include_directories(core_context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
