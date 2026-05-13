# 数据库访问层，项目相关
# 收集源文件和头文件

set(SOURCES
)
set(HEADERS
)

# 创建对象库
add_library(core_dao_project STATIC
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_dao_project PRIVATE
    CoreDepends
    core_interfaces
    core_dao_interfaces
)
