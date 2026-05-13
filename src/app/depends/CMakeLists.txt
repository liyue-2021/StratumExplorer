# 主程序专属依赖
add_library(AppDepends INTERFACE)
target_link_libraries(AppDepends INTERFACE
    GlobalDepends
    Qt::Core
    Qt::Widgets
)

if(WITH_HDF5)
    target_link_libraries(AppDepends INTERFACE hdf5_cpp-static)
endif()
