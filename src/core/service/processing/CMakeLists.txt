# 工作流引擎实现
set(SOURCES
    NodeRegistry.cpp
    WorkflowEngine.cpp
    WorkflowTypes.cpp
    DemoNodes.cpp
    ExternalProcessRunner.cpp
    ExternalProcessNode.cpp
    LogBus.cpp
)
set(HEADERS
    NodeRegistry.h
    WorkflowEngine.h
    DemoNodes.h
    ExternalProcessRunner.h
    ExternalProcessNode.h
    LogBus.h
)

add_library(core_service_processing STATIC
    ${SOURCES}
    ${HEADERS}
)

set_target_properties(core_service_processing PROPERTIES AUTOMOC ON)

target_include_directories(core_service_processing PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_service_processing PUBLIC
    Qt::Core
    Qt::Concurrent
    core_interfaces
    core_interfaces_models_processing
    core_interfaces_service_processing
)
