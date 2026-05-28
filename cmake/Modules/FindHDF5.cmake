# Fetch zlib + HDF5 (static). zlib is required for deflate-compressed .h5 from backend EXE.
# Prefer manual drops under <repo>/build/zlib-1.3.1 and build/hdf5-2.1.0 (no network).
include(FetchContent)

set(_OILPRO_ZLIB_LOCAL "${CMAKE_SOURCE_DIR}/build/zlib-1.3.1")
set(_OILPRO_HDF5_LOCAL "${CMAKE_SOURCE_DIR}/build/hdf5-2.1.0")

# ---------------- zlib ----------------
if(NOT TARGET ZLIB::ZLIB AND NOT TARGET zlibstatic)
    if(EXISTS "${_OILPRO_ZLIB_LOCAL}/CMakeLists.txt")
        message(STATUS "Using local zlib: ${_OILPRO_ZLIB_LOCAL}")
        FetchContent_Declare(zlib_fc SOURCE_DIR "${_OILPRO_ZLIB_LOCAL}")
    else()
        message(STATUS "Fetching zlib for HDF5 deflate support...")
        FetchContent_Declare(
            zlib_fc
            URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
    endif()
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zlib_fc)
endif()

if(TARGET zlibstatic)
    set(_zlib_tgt zlibstatic)
elseif(TARGET zlib)
    set(_zlib_tgt zlib)
else()
    message(FATAL_ERROR "zlib FetchContent did not define zlibstatic/zlib target")
endif()

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS ${_zlib_tgt})
endif()

get_target_property(_zlib_inc ${_zlib_tgt} INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _zlib_inc)
    get_target_property(_zlib_inc ${_zlib_tgt} INCLUDE_DIRECTORIES)
endif()
if(NOT _zlib_inc)
    set(_zlib_inc "${zlib_fc_SOURCE_DIR};${zlib_fc_BINARY_DIR}")
endif()

set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
set(ZLIB_INCLUDE_DIR "${_zlib_inc}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARIES ${_zlib_tgt} CACHE STRING "" FORCE)
set(ZLIB_LIBRARY ${_zlib_tgt} CACHE STRING "" FORCE)

# Tell HDF5 2.x that zlib is already available (skip FindZLIB)
set(_zlib_src_dir "${zlib_fc_SOURCE_DIR}")
if(NOT _zlib_src_dir)
    set(_zlib_src_dir "${_OILPRO_ZLIB_LOCAL}")
endif()
set(H5_ZLIB_HEADER "zlib.h" CACHE STRING "oilPro provides zlib" FORCE)
set(H5_ZLIB_INCLUDE_DIR_GEN "${_zlib_src_dir}" CACHE PATH "" FORCE)
set(H5_ZLIB_INCLUDE_DIRS "${_zlib_src_dir}" CACHE STRING "" FORCE)

# ---------------- HDF5 ----------------
set(HDF5_TAG 2.1.0)
set(HDF5_EXTERNALLY_CONFIGURED ON CACHE BOOL "subproject: skip broken export()" FORCE)
set(HDF5_EXPORTED_TARGETS "" CACHE STRING "no install export from FetchContent" FORCE)
set(HDF5_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_CPP_LIB ON CACHE BOOL "" FORCE)
set(HDF5_BUILD_HL_LIB OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
# HDF5 2.1 option name (NOT HDF5_ENABLE_Z_LIB_SUPPORT)
set(HDF5_ENABLE_ZLIB_SUPPORT ON CACHE BOOL "zlib for compressed datasets" FORCE)
set(HDF5_ENABLE_SZIP_SUPPORT OFF CACHE BOOL "" FORCE)
set(HDF5_ENABLE_HDFS OFF CACHE BOOL "no Java/JNI required" FORCE)
set(HDF5_BUILD_JAVA OFF CACHE BOOL "" FORCE)
set(HDF5_USE_COMPILER_COMPLEX OFF CACHE BOOL "" FORCE)
set(HDF5_ENABLE_PARALLEL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CMAKE_C_STANDARD 99 CACHE STRING "" FORCE)
set(HDF5_ENABLE_DEBUG_CALLSTACK OFF CACHE BOOL "" FORCE)
set(HDF5_ENABLE_SYMBOLS OFF CACHE BOOL "" FORCE)
set(HDF5_ALLOW_EXTERNAL_SUPPORT OFF CACHE BOOL "" FORCE)

if(EXISTS "${_OILPRO_HDF5_LOCAL}/CMakeLists.txt")
    message(STATUS "Using local HDF5: ${_OILPRO_HDF5_LOCAL} (ZLIB=${_zlib_tgt})")
    FetchContent_Declare(
        hdf5
        SOURCE_DIR "${_OILPRO_HDF5_LOCAL}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/hdf5-build"
    )
else()
    message(STATUS "Fetching HDF5 ${HDF5_TAG} (ZLIB=${_zlib_tgt})...")
    FetchContent_Declare(
        hdf5
        URL https://github.com/HDFGroup/hdf5/archive/refs/tags/${HDF5_TAG}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()
set(HDF5_LIB_DEPENDENCIES "${_zlib_tgt}" CACHE STRING "zlib for HDF5 deflate" FORCE)
FetchContent_MakeAvailable(hdf5)

foreach(_h5_tgt IN ITEMS hdf5-static hdf5-shared hdf5_cpp-static hdf5_cpp-shared)
    if(TARGET ${_h5_tgt})
        target_include_directories(${_h5_tgt} PUBLIC "${_zlib_src_dir}")
        target_link_libraries(${_h5_tgt} PUBLIC ${_zlib_tgt})
    endif()
endforeach()

message(STATUS "HDF5 ready: hdf5_cpp-static + ${_zlib_tgt} (HDF5_ENABLE_ZLIB_SUPPORT=ON)")
