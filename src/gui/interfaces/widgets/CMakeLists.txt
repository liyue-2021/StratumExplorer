# ==============================================
# 模块：gui/widgets 接口层
# 拆分 2 个子模块：
# 1. base：纯 C++ 抽象接口（无任何 Qt 依赖）
# 2. qt：  Qt 专属接口（依赖 Qt 元对象/信号槽）
# ==============================================

# --------------------------
# 子模块1：纯 C++ 接口库（核心业务接口，无Qt）
# 存放：IDataOperateInterface 等纯虚函数接口
# --------------------------
add_library(gui_interfaces_widgets_base INTERFACE
        IDataOperateInterface.h
        DataOperateTools.h
)
target_link_libraries(gui_interfaces_widgets_base INTERFACE
        Qt::Core
)

# 纯C++接口 → 无任何Qt依赖
# target_link_libraries 留空

# --------------------------
# 子模块2：Qt 接口库（依赖Qt，用于信号槽/元对象）
# 存放：Qt专用接口、QObject接口、插件接口
# --------------------------
add_library(gui_interfaces_widgets_qt STATIC
        BaseDataOperateWidget.h
        BaseDataOperateWidget.cpp
        BaseDataOperateDialog.h
        BaseDataOperateDialog.cpp
        BaseDataOperateMainWindow.h
        BaseDataOperateMainWindow.cpp
)

# Qt 接口依赖 Qt Core
target_link_libraries(gui_interfaces_widgets_qt PUBLIC
        Qt::Core
        Qt::Widgets
)

# --------------------------
# 聚合库：对外统一入口（推荐！）
# 外部只需链接这一个库，自动获得 base + qt 能力
# --------------------------
add_library(gui_interfaces_widgets INTERFACE
)

target_link_libraries(gui_interfaces_widgets INTERFACE
        gui_interfaces_widgets_base   # 纯C++接口
        gui_interfaces_widgets_qt     # Qt接口
)

# 导出头文件路径（与base保持一致）
target_include_directories(gui_interfaces_widgets INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
