# 工作流相关数据模型（接口层）
add_library(core_interfaces_models_processing INTERFACE)

target_sources(core_interfaces_models_processing INTERFACE
    WorkflowTypes.h
    NodeDefinition.h
)

target_include_directories(core_interfaces_models_processing INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(core_interfaces_models_processing INTERFACE
    Qt::Core
)
