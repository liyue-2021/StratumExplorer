# widget组件
# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)
set(FORMS
)

# 创建对象库
add_library(gui_widgets STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# 添加头文件包含路径
target_include_directories(gui_widgets INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 指定 gui_widgets 库需要链接的其他库
target_link_libraries(gui_widgets PRIVATE
    GuiDepends
)
