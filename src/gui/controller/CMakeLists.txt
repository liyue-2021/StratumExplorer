# 控制层，接收UI信号并调用Service处理业务逻辑，再将结果返回给UI进行显示
# 收集源文件和头文件
set(SOURCES
    system/LicenseController.cpp
)
set(HEADERS
    system/LicenseController.h
)
set(FORMS
)

# 创建对象库
add_library(gui_controller STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(gui_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gui_controller PRIVATE
    GuiDepends
    core
    gui_interfaces
    # gui_adapter
)
