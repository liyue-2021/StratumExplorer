#=========================================================
# StratumExplorer - 测井解释软件
# 根CMake：仅做项目入口 + 全局调度（禁止写业务逻辑）
#=========================================================
cmake_minimum_required(VERSION 3.26)

# 兼容老的子项目（如 HDF5 2.1.0 内部要求的 cmake 版本被 CMake 4.0 移除支持时使用）
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Min policy for sub-projects")

project(StratumExplorer
        VERSION 1.0.0
        DESCRIPTION "测井解释软件"
        LANGUAGES CXX
)

#全局参数
include(FetchContent)

#==================== 全局基础配置 ====================
# 加载自定义模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# 加载【项目通用设置】：输出目录、系统架构、编译选项、全局配置
include(ProjectSetup)

#==================== 功能开关 ====================
option(WITH_TEST "Enable TESTING" OFF)
option(WITH_HDF5 "Enable HDF5 support" ON)

#==================== 加载依赖与模块 ====================
include(CheckVcpkg)       # 检查vcpkg
include(BasePackages)     # 基础依赖包
include(BaseConfig)       # 编译配置
include(FindQtTranslations) # Qt翻译

#==================== 翻译文件 ====================
set(TRANSLATION_TS_FILES
        translations/global_en_us.ts
        translations/global_zh_cn.ts
)
qt_generate_translations(generate_qm
        TS_FILES ${TRANSLATION_TS_FILES}
        OUTPUT_DIR ${CMAKE_SOURCE_DIR}/translations
)
# # 更新 .ts 文件的目标
# qt_update_translations(update_translations
#     SOURCES src/
#     TS_FILES ${TRANSLATION_TS_FILES}
# )
# # 新增：打开 Qt Linguist 的目标
# qt_open_translations_in_linguist(open_linguist
#     TS_FILES ${TRANSLATION_TS_FILES}
# )

#==================== HDF5 支持 ====================
if(WITH_HDF5)
    include(FindHDF5)
    add_compile_definitions(WITH_HDF5)
endif()

#==================== 调度子目录 ====================
add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(platform)

if(WITH_TEST)
    add_subdirectory(test)
endif()

# 工具链支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
