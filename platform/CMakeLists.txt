# 1. 定义目标平台变量，默认为当前系统平台
if(NOT DEFINED TARGET_PLATFORM)
    if(WIN32)
        set(TARGET_PLATFORM "Windows" CACHE STRING "Target platform")
    elseif(APPLE)
        set(TARGET_PLATFORM "Darwin" CACHE STRING "Target platform")
    elseif(UNIX)
        set(TARGET_PLATFORM "Linux" CACHE STRING "Target platform")
    else()
        set(TARGET_PLATFORM "Unknown" CACHE STRING "Target platform")
    endif()
endif()

message(STATUS "Target platform: ${TARGET_PLATFORM}")

add_subdirectory(windows)
add_subdirectory(mac)
add_subdirectory(linux)
