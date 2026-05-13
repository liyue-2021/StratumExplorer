include(FetchContent)

# ====================== HDF5 缓存目录 ======================
#set(HDF5_CACHE_ROOT ${DEPS_CACHE_ROOT}/hdf5)

# ====================== HDF5 编译选项 ======================
set(HDF5_TAG 2.1.0)
set(HDF5_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(HDF5_BUILD_CPP_LIB ON CACHE BOOL "Build C++ library" FORCE)
set(HDF5_BUILD_HL_LIB OFF CACHE BOOL "Build high-level library" FORCE)
set(HDF5_ENABLE_Z_LIB_SUPPORT OFF CACHE BOOL "zlib" FORCE)
set(HDF5_ENABLE_SZIP_SUPPORT OFF CACHE BOOL "szip" FORCE)
#MSVC不支持以下的选项
set(HDF5_USE_COMPILER_COMPLEX OFF CACHE BOOL "Disable complex type (fix MSVC compile error)" FORCE)  # 关闭复数（解决语法错误）
set(HDF5_ENABLE_PARALLEL OFF CACHE BOOL "Disable MPI" FORCE)  # 关闭MPI（解决H5mpi.c报错）
set(CMAKE_C_STANDARD 99 CACHE STRING "" FORCE)  # 降级C99，MSVC对C11支持烂
set(HDF5_ENABLE_DEBUG_CALLSTACK OFF CACHE BOOL "" FORCE)  # 关闭调试栈
set(HDF5_ENABLE_SYMBOLS OFF CACHE BOOL "" FORCE)  # 关闭系统符号

# ====================== 拉取 + 编译 + 安装 ======================
FetchContent_Declare(
        hdf5
        GIT_REPOSITORY https://github.com/HDFGroup/hdf5.git
        GIT_TAG ${HDF5_TAG}
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(hdf5)
