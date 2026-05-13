# 收集源文件和头文件
set(SOURCES

)
set(HEADERS
)

# 创建对象库
add_library(core_utils STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(core_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(core_utils PRIVATE
    CoreDepends
)

target_link_libraries(core_utils PUBLIC
    core_interfaces
)
