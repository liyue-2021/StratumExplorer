# 工作流引擎实现
set(SOURCES
    NodeRegistry.cpp
    WorkflowEngine.cpp
    WorkflowTypes.cpp
    DemoNodes.cpp
    ProductionNodes.cpp
    ProductionNodeParams.cpp
    ExternalProcessRunner.cpp
    ExternalProcessNode.cpp
    ParamRangePair.cpp
    NodeCompatibility.cpp
    PlotData.cpp
    LogBus.cpp
    node_client_params.qrc
)
set(HEADERS
    NodeRegistry.h
    WorkflowEngine.h
    DemoNodes.h
    ProductionNodes.h
    ProductionNodeParams.h
    ExternalProcessRunner.h
    ExternalProcessNode.h
    ParamRangePair.h
    NodeCompatibility.h
    PlotData.h
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

if(WITH_HDF5)
    target_link_libraries(core_service_processing PUBLIC CoreDepends)
endif()
