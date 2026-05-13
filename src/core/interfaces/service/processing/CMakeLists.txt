# 工作流相关服务接口
# 注意：IWorkflowEngine 含 Q_OBJECT，必须在 STATIC 库里 MOC 一次，
# 否则每个使用方会重复 MOC，链接时报 LNK2005。
add_library(core_interfaces_service_processing STATIC
    moc_anchor.cpp
    IProcessNode.h
    INodeFactory.h
    IWorkflowEngine.h
)

target_include_directories(core_interfaces_service_processing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(core_interfaces_service_processing PUBLIC
    Qt::Core
    core_interfaces_models_processing
)
