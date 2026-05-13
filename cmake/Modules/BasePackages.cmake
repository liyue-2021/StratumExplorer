# 最低 Qt6 版本（项目锁定 Qt 6.10）
if(NOT DEFINED minQt6Version)
    set(minQt6Version 6.5)
endif()

find_package(Qt6 ${minQt6Version} REQUIRED
    COMPONENTS Core Network Sql Xml Charts Concurrent LinguistTools
)
#TODO widget相关依赖放在gui子模块中
find_package(Qt6 ${minQt6Version} REQUIRED COMPONENTS Widgets Qml Quick)
