# gui相关工具类
# 收集源文件和头文件
set(SOURCES
    ThemeManager.cpp
)
set(HEADERS
    ThemeManager.h
)

# 创建对象库
add_library(gui_utils STATIC
    ${SOURCES}
    ${HEADERS}
)

# 添加头文件包含路径
target_include_directories(gui_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 指定 gui_utils 库需要链接的其他库
target_link_libraries(gui_utils PRIVATE
    GuiDepends
)
