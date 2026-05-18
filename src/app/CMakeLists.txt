#程序入口
add_subdirectory(depends)

set(QRC_FILES
    ${CMAKE_SOURCE_DIR}/resources/qrc/font.qrc
    # TODO 路径不要有中文
    ${CMAKE_SOURCE_DIR}/resources/qrc/icons.qrc
    ${CMAKE_SOURCE_DIR}/resources/qrc/image.qrc
    ${CMAKE_SOURCE_DIR}/resources/qrc/styles.qrc
    # TODO 不打包进程序，动态读取
    ${CMAKE_SOURCE_DIR}/translations/translations.qrc
)

if(WIN32)
    add_executable(app WIN32)
    if(MSVC)
        set_target_properties(app PROPERTIES
                LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
                LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        )
    endif()
else()
    add_executable(app)
endif()

target_sources(app PRIVATE
    main.cpp
    Application.cpp
    Application.h
    ${QRC_FILES}
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(app PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

target_link_libraries(app PRIVATE
    core
    AppDepends
)

# TODO 动态加载widgets
target_link_libraries(app PRIVATE
    gui
)

# 确保构建 app 前先生成 .qm 翻译文件（被 translations.qrc 引用）
add_dependencies(app generate_qm)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt6 windeployqt
            HINTS "${Qt6_DIR}/bin"
            REQUIRED
    )

    add_custom_command(TARGET app POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
            --verbose 1
            --no-compiler-runtime
            --no-opengl-sw
            $<TARGET_FILE:app>
            COMMENT "正在自动复制 Qt 依赖库到 exe 目录..."
    )
endif()

# 节点参数表：随 exe 部署一份，供 QRC 未链入时的兜底读取
add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/core/service/processing/node_client_params.json
        $<TARGET_FILE_DIR:app>/node_client_params.json
        COMMENT "Copy node_client_params.json beside executable"
)
