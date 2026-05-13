#所有核心代码（ui无关）
add_library(core STATIC
)

add_subdirectory(config)
add_subdirectory(context)
add_subdirectory(dao)
add_subdirectory(depends)
add_subdirectory(interfaces)
add_subdirectory(models)
add_subdirectory(service)
add_subdirectory(utils)

target_link_libraries(core PUBLIC
    core_interfaces
    CoreDepends
)

target_link_libraries(core PRIVATE
    core_config
    core_context
    core_dao
    core_models
    core_service
    core_utils
)

