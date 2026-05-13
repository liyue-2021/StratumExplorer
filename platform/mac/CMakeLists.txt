if(TARGET_PLATFORM STREQUAL "Darwin")
    # 添加 macOS 专用源文件（如 Objective-C++）
    # target_sources(app PRIVATE mac_specific.mm)
    # 设置 macOS 应用图标等
endif()
