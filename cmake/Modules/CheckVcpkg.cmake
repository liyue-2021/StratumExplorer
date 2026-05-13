# cmake/Modules/CheckVcpkg.cmake
# 检测是否使用了 vcpkg 工具链，并设置全局变量 VCPKG_ENABLED 和 VCPKG_ROOT

# if(DEFINED VCPKG_CHECKED)
    # return()
# endif()
# set(VCPKG_CHECKED TRUE CACHE INTERNAL "Flag indicating vcpkg check has been performed")

if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    # 提取 vcpkg 根目录：从工具链文件路径中解析出 vcpkg 安装根目录
    get_filename_component(VCPKG_SCRIPTS_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)   # .../scripts/buildsystems
    get_filename_component(VCPKG_ROOT "${VCPKG_SCRIPTS_DIR}" DIRECTORY)             # .../scripts
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)                   # .../vcpkg
    message(STATUS "Using vcpkg from: ${VCPKG_ROOT}")
    set(VCPKG_ENABLED TRUE CACHE INTERNAL "vcpkg is used")
    set(VCPKG_ROOT "${VCPKG_ROOT}" CACHE INTERNAL "vcpkg installation root")
else()
    message(WARNING "vcpkg toolchain not detected. It is recommended to use vcpkg for managing dependencies. "
                    "Set CMAKE_TOOLCHAIN_FILE to the vcpkg toolchain file (e.g., "
                    "-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake).")
    set(VCPKG_ENABLED FALSE CACHE INTERNAL "vcpkg is not used")
endif()
