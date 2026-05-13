# FindQtTranslations.cmake
# 提供 Qt 翻译文件管理的 CMake 函数

if(NOT TARGET Qt6::lupdate OR NOT TARGET Qt6::lrelease)
    find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
endif()

#------------------------------------------------------------------------------
# qt_update_translations(<target> SOURCES <src_dirs> TS_FILES <ts_files>)
# 创建自定义目标 <target>，调用 lupdate 从源目录更新 .ts 文件。
# 所有路径均转换为绝对路径（相对于调用者的源目录）。
#------------------------------------------------------------------------------
function(qt_update_translations target_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs SOURCES TS_FILES)
    cmake_parse_arguments(ARGS "" "" "SOURCES;TS_FILES" ${ARGN})

    if(NOT ARGS_SOURCES)
        message(FATAL_ERROR "qt_update_translations: SOURCES must be specified")
    endif()
    if(NOT ARGS_TS_FILES)
        message(FATAL_ERROR "qt_update_translations: TS_FILES must be specified")
    endif()

    # 将源目录和 .ts 文件转换为绝对路径（基于调用者的源目录）
    set(abs_sources)
    foreach(src ${ARGS_SOURCES})
        get_filename_component(abs_src ${src} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        list(APPEND abs_sources ${abs_src})
    endforeach()

    set(abs_ts_files)
    foreach(ts ${ARGS_TS_FILES})
        get_filename_component(abs_ts ${ts} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        list(APPEND abs_ts_files ${abs_ts})
    endforeach()

    # 构建 lupdate 命令（使用绝对路径）
    add_custom_target(${target_name}
        COMMAND Qt6::lupdate -recursive ${abs_sources} -ts ${abs_ts_files}
        COMMENT "Updating translation files from ${abs_sources}"
        VERBATIM
    )
endfunction()

#------------------------------------------------------------------------------
# qt_generate_translations(<target> TS_FILES <ts_files> [OUTPUT_DIR <dir>])
# 创建自定义目标 <target>，为每个 .ts 文件生成对应的 .qm 文件。
# .ts 文件路径会转换为绝对路径，输出目录若为相对则转为绝对路径。
#------------------------------------------------------------------------------
function(qt_generate_translations target_name)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs TS_FILES)
    cmake_parse_arguments(ARGS "" "OUTPUT_DIR" "TS_FILES" ${ARGN})

    if(NOT ARGS_TS_FILES)
        message(FATAL_ERROR "qt_generate_translations: TS_FILES must be specified")
    endif()

    # 将 .ts 文件转换为绝对路径
    set(abs_ts_files)
    foreach(ts ${ARGS_TS_FILES})
        get_filename_component(abs_ts ${ts} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        list(APPEND abs_ts_files ${abs_ts})
    endforeach()

    # 处理输出目录：如果指定且为相对路径，则转为绝对路径
    if(ARGS_OUTPUT_DIR)
        get_filename_component(output_dir ${ARGS_OUTPUT_DIR} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    else()
        set(output_dir "")
    endif()

    set(qm_files)
    foreach(ts ${abs_ts_files})
        get_filename_component(name_we ${ts} NAME_WE)
        if(output_dir)
            set(qm_file ${output_dir}/${name_we}.qm)
        else()
            get_filename_component(ts_dir ${ts} DIRECTORY)
            set(qm_file ${ts_dir}/${name_we}.qm)
        endif()

        add_custom_command(
            OUTPUT ${qm_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
            COMMAND Qt6::lrelease ${ts} -qm ${qm_file}
            DEPENDS ${ts}
            COMMENT "Generating ${qm_file}"
            VERBATIM
        )
        list(APPEND qm_files ${qm_file})
    endforeach()

    add_custom_target(${target_name} DEPENDS ${qm_files})
endfunction()

# qt_open_translations_in_linguist(<target> TS_FILES <ts_files>)
# 创建一个自定义目标，当构建时打开 Qt Linguist 并加载指定的 .ts 文件。
function(qt_open_translations_in_linguist target_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs TS_FILES)
    cmake_parse_arguments(ARGS "" "" "TS_FILES" ${ARGN})

    if(NOT ARGS_TS_FILES)
        message(FATAL_ERROR "qt_open_translations_in_linguist: TS_FILES must be specified")
    endif()

    # 查找 Qt Linguist 可执行文件
    if(NOT QT_LINGUIST_EXECUTABLE)
        # 尝试从 Qt6_DIR 获取路径
        if(Qt6_DIR)
            get_filename_component(QT_BIN_DIR ${Qt6_DIR}/../bin ABSOLUTE)
            set(QT_LINGUIST_EXECUTABLE "${QT_BIN_DIR}/linguist${CMAKE_EXECUTABLE_SUFFIX}")
        else()
            # 降级使用 find_program
            find_program(QT_LINGUIST_EXECUTABLE linguist)
        endif()
        if(NOT EXISTS ${QT_LINGUIST_EXECUTABLE})
            message(WARNING "Qt Linguist not found. Please ensure Qt6 is installed and linguist is available.")
            return()
        endif()
    endif()

    # 将 TS 文件转换为绝对路径
    set(abs_ts_files)
    foreach(ts ${ARGS_TS_FILES})
        get_filename_component(abs_ts ${ts} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        list(APPEND abs_ts_files ${abs_ts})
    endforeach()

    # 创建自定义目标
    add_custom_target(${target_name}
        COMMAND ${QT_LINGUIST_EXECUTABLE} ${abs_ts_files}
        COMMENT "Opening Qt Linguist with ${abs_ts_files}"
        VERBATIM
    )
endfunction()
