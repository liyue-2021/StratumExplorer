# 工作流编辑器子模块
set(SOURCES
    WorkflowEditorTab.cpp
    WorkflowScene.cpp
    WorkflowView.cpp
    NodeItem.cpp
    EdgeItem.cpp
    NodePalette.cpp
    PlotDialog.cpp
    FormatConvertDialog.cpp
    DepthCorrectDialog.cpp
    TimeCorrectDialog.cpp
    DataCropDialog.cpp
    DataMergeDialog.cpp
    DasConvertDialog.cpp
    LfdasExtractDialog.cpp
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
    FormatConvertDialog.h
    DepthCorrectDialog.h
    TimeCorrectDialog.h
    DataCropDialog.h
    DataMergeDialog.h
    DasConvertDialog.h
    LfdasExtractDialog.h
    LogDock.h
)
set(FORMS
    FormatConvertDialog.ui
    DepthCorrectDialog.ui
    TimeCorrectDialog.ui
    DataCropDialog.ui
    DataMergeDialog.ui
    DasConvertDialog.ui
    LfdasExtractDialog.ui
)

add_library(gui_workspace_processing_workflow STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
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
