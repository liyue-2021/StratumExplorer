# core 专属依赖（独立管理，其他模块不可见）
add_library(CoreDepends INTERFACE)
# 继承全局Qt + 可添加core专用第三方库
target_link_libraries(CoreDepends INTERFACE
    GlobalDepends
    Qt::Core
    Qt::Sql
    Qt::Network
    Qt::Concurrent
)

if(WITH_HDF5)
    target_link_libraries(CoreDepends INTERFACE hdf5_cpp-static)
endif()
