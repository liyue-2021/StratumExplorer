# 定义数据处理的信号，gui通过槽函数更新ui
# 创建一个接口库，仅用于收集头文件路径
file(GLOB_RECURSE DATA_SOURCES "*.cpp")
file(GLOB_RECURSE DATA_HEADERS "*.h")
add_library(core_interfaces_dao INTERFACE
#    DatabaseManager.h
#    BaseDao.h
    ${DATA_SOURCES}
    ${DATA_HEADERS}
#    system/ILicenseDao.h
#    system/IUserDao.h
)

# 需要依赖的其他库
target_link_libraries(core_interfaces_dao INTERFACE
    Qt::Core
    core_interfaces_models
)

target_include_directories(core_interfaces_dao INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
