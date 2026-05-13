# 收集源文件和头文件
set(SOURCES
    system/LicenseServiceImpl.cpp
    system/UserServiceImpl.cpp
#    data/DataService.cpp
)
set(HEADERS
    system/LicenseServiceImpl.h
    system/UserServiceImpl.h
#    data/DataService.h
)

# 创建对象库
add_library(core_service STATIC
    ${SOURCES}
    ${HEADERS}
)

# 子模块（工作流引擎实现）
add_subdirectory(processing)

target_include_directories(core_service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(core_service PUBLIC
    core_service_processing
)

target_link_libraries(core_service PRIVATE
    CoreDepends
    core_interfaces
    #TODO 寻找不需要显示引入的方法
    core_interfaces_service_qt
    core_config
    core_dao
)
