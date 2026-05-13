# 数据访问层，系统相关
# 收集源文件和头文件
set(SOURCES
    LicenseDaoImpl.cpp
    UserDaoImpl.cpp
)
set(HEADERS
    LicenseDaoImpl.h
    UserDaoImpl.h
)

# 创建对象库
add_library(core_dao_system STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_dao_system PRIVATE
    CoreDepends
    core_interfaces
    core_dao_interfaces
)
