#封装service，用不到了
# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)
set(FORMS
)

# 创建对象库
add_library(gui_adapter STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(gui_adapter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gui_adapter PRIVATE
    GuiDepends
    core
    gui_interfaces
)
