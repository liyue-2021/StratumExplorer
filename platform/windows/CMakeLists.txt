if(TARGET_PLATFORM STREQUAL "Windows")
    # 添加 Windows 专用资源
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/winrc/icon.rc
    )
    # 添加其他 Windows 源文件
    # target_sources(app PRIVATE win_specific.cpp)
    if(MSVC)
        target_compile_options(app PRIVATE /FS)
    endif()
endif()
