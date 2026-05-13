# 控制层，接收UI信号并调用Service处理业务逻辑，再将结果返回给UI进行显示
# 收集源文件和头文件
set(SOURCES
    GuiContext.cpp
    GuiContextFactory.cpp
)
set(HEADERS
    GuiContext.h
)

# 创建对象库
add_library(gui_context STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(gui_context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gui_context PRIVATE
    GuiDepends
    core
    gui_interfaces
)
