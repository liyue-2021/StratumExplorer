# QCustomPlot 2.1.1 — https://www.qcustomplot.com
# 双协议 GPL/Commercial。集成到本项目按 GPL 处理；如需商用请向作者购买授权。

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)

add_library(qcustomplot STATIC
    qcustomplot.cpp
    qcustomplot.h
)

set_target_properties(qcustomplot PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
)

target_include_directories(qcustomplot PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(qcustomplot PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

target_compile_definitions(qcustomplot PUBLIC QT_NO_PRINTER=0)
# 注意：不要定义 QCUSTOMPLOT_USE_LIBRARY 和 QCUSTOMPLOT_COMPILE_LIBRARY；
# 二者只要任意 #if defined 命中就会强制 dllimport/dllexport，
# 我们这里是直接静态编译，保持空 QCP_LIB_DECL。

# 关闭这份第三方代码的过严警告，避免污染主工程构建日志
if (MSVC)
    target_compile_options(qcustomplot PRIVATE /W1 /wd4996)
else()
    target_compile_options(qcustomplot PRIVATE -w)
endif()

add_library(third_party::qcustomplot ALIAS qcustomplot)
