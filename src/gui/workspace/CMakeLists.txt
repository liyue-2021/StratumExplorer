#主窗口中间的大模块，如实时数据，预处理
add_subdirectory(processing)
add_subdirectory(realtime)
add_subdirectory(device)
add_subdirectory(well)

set(SOURCES
    WorkSpacePage.cpp
)
set(HEADERS
    WorkSpacePage.h
)
set(FORMS
    WorkSpacePage.ui
)


# 创建对象库
add_library(gui_workspace STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(gui_workspace PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(gui_workspace PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 指定 gui_mainwindow 库需要链接的其他库
target_link_libraries(gui_workspace PRIVATE
    GuiDepends
    gui_workspace_processing
    gui_workspace_realtime
    gui_workspace_device
    gui_workspace_well
    gui_interfaces
    gui_controller
)
