# 收集源文件和头文件
set(SOURCES
)
set(HEADERS
)
set(FORMS
)

# 创建对象库
add_library(gui_guiservice STATIC
    ${SOURCES}
    ${HEADERS}
    ${FORMS}

)



target_link_libraries(gui_guiservice PRIVATE
    GuiDepends
    gui_interfaces
    core
)
