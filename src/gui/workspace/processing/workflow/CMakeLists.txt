# 工作流编辑器子模块
set(SOURCES
    WorkflowEditorTab.cpp
    WorkflowScene.cpp
    WorkflowView.cpp
    NodeItem.cpp
    EdgeItem.cpp
    NodePalette.cpp
    PlotDialog.cpp
    LogDock.cpp
)
set(HEADERS
    WorkflowEditorTab.h
    WorkflowScene.h
    WorkflowView.h
    NodeItem.h
    EdgeItem.h
    NodePalette.h
    PlotDialog.h
    LogDock.h
)

add_library(gui_workspace_processing_workflow STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(gui_workspace_processing_workflow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gui_workspace_processing_workflow PUBLIC
    GuiDepends
    core_interfaces_models_processing
    core_interfaces_service_processing
    core_service_processing
    qcustomplot
)
