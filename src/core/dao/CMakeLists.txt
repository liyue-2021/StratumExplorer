#数据库访问层
add_subdirectory(interfaces)
add_subdirectory(project)
add_subdirectory(system)

set(SOURCES
    DatabaseManager.cpp   
)

set(HEADERS
)

# 创建对象库
add_library(core_dao STATIC

)

target_sources(core_dao PRIVATE
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(core_dao PRIVATE
    CoreDepends
    core_interfaces
    core_dao_system
    core_dao_project
)

target_include_directories(core_dao PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
