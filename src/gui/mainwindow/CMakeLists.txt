#主窗口和欢迎页
add_subdirectory(dialogs)
add_subdirectory(menus)
# 收集源文件和头文件
set(SOURCES
    MainWindow.cpp
    WelcomePage.cpp
)
set(HEADERS
    MainWindow.h
    WelcomePage.h
)
set(FORMS
    MainWindow.ui
    WelcomePage.ui
)

# 创建对象库
add_library(gui_mainwindow STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(gui_mainwindow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 指定 gui_mainwindow 库需要链接的其他库
target_link_libraries(gui_mainwindow PRIVATE
    GuiDepends
    core
    gui_mainwindow_dialogs
    gui_mainwindow_menus
    gui_workspace
    gui_controller
    gui_interfaces
)
