# 本地读取后端 HDF5（含 zlib 压缩）

## 力炜那边为啥报错

后端 EXE 写的 `processed_*.h5` 若用了 **deflate 压缩**，HDF5 必须链接 **zlib**。  
MSVC 上经常出现「zlib 动态/静态都链不进去」——本质是 **HDF5 编译时没开 ZLIB，或链错了库**。

HDF5 **2.1** 的正确 CMake 开关是 **`HDF5_ENABLE_ZLIB_SUPPORT`**（不是旧文档里的 `HDF5_ENABLE_Z_LIB_SUPPORT`）。写错时 HDF5 会编译成功但 **没有 deflate**，读 GZIP 的 `processed_*.h5` 会失败。

## 你这边自己解析 —— 可行吗？

**可以。** 推荐做法：

1. 用本仓库 **打开 HDF5 支持** 的构建（CMake 自动 Fetch **zlib + HDF5**，并打开 deflate）。
2. 在 `core` 用力炜的 `PlotData`（`IPlotData`）读 `/Processed/Data`，再喂给 `PlotDialog`。

不必再单独「系统安装」一套 HDF5，除非你想用 vcpkg 全局装。

## 手动放置源码（免下载，推荐网络差时用）

将压缩包解压到仓库根下（路径必须一致）：

- `build/zlib-1.3.1/` — 根目录有 `zlib.h`、`CMakeLists.txt`
- `build/hdf5-2.1.0/` — 根目录有 `CMakeLists.txt`、`src/`、`c++/`

然后执行 `clean_hdf5_deps.cmd` 再 `build_oilpro_hdf5.cmd`，CMake 会优先用本地目录。

## 一键构建（HDF5 版）

```bat
tools\build\build_oilpro_hdf5.cmd
```

- 输出目录：`build/Desktop_Qt_6_10_3_MSVC2022_64bit-Debug-HDF5`
- **首次配置会下载并编译 zlib + HDF5，约 10～20 分钟**，请看 `tools/build/logs/build_oilpro_hdf5.log`
- 若日志长时间停在 `Fetching HDF5 2.1.0` 且 `_deps/hdf5-src` 几乎是空的：Git 克隆卡住了。先运行 `tools\build\clean_hdf5_deps.cmd`，再重新 `build_oilpro_hdf5.cmd`（已改为压缩包下载，不再走 git clone）
- 若报错 `Could NOT find JNI`：在 Qt Creator 里把 **`HDF5_ENABLE_HDFS` 设为 `OFF`**（不要 ON，会要求装 Java），然后 **清除 CMake 配置** 再重新运行 CMake。工程已在 `FindHDF5.cmake` 里默认关闭 HDFS。
- 日常快速构建仍用 `build_oilpro.cmd`（`WITH_HDF5=OFF`）

## 代码入口

| 文件 | 作用 |
|------|------|
| `cmake/Modules/FindHDF5.cmake` | Fetch zlib → 再 Fetch HDF5（`HDF5_ENABLE_Z_LIB_SUPPORT ON`） |
| `src/core/interfaces/service/processing/IPlotData.h` | 力炜绘图数据接口 |
| `src/core/service/processing/PlotData.h/.cpp` | `LoadPlotData` 读 `/Processed/Data` |
| `PlotDialog` | `loadResultFromPlotService()` 接 PlotData 结果画热力图 |

示例（在任意已链接 `core_service_processing` 的模块）：

```cpp
#include "PlotData.h"

PlotRequest req;
req.h5Path = h5Path;
req.kind = QStringLiteral("Overview");
PlotDisplayData out;
QString err;
PlotData().LoadPlotData(req, &out, &err);
```

## 可选：vcpkg 全局安装

若更想用系统级依赖：

```bat
git clone https://github.com/microsoft/vcpkg %USERPROFILE%\vcpkg
%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat
%USERPROFILE%\vcpkg\vcpkg install zlib:x64-windows hdf5[zlib]:x64-windows
```

配置时加：

```bat
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%USERPROFILE%/vcpkg/scripts/buildsystems/vcpkg.cmake ...
```

并改 `FindHDF5.cmake` 为 `find_package(HDF5 CONFIG REQUIRED)`（与当前 FetchContent 二选一）。

## 和力炜的分工建议

| 谁 | 做什么 |
|----|--------|
| 力炜 | 继续写 `processed_*.h5`；在文档里写明 **数据集路径、行列含义、是否压缩** |
| 你 | `PlotData` + `PlotDialog`；用 **同一套带 zlib 的 HDF5** 读他的文件 |
| 避免 | 后端再嵌一套 HDF5 却关 zlib；前端关 HDF5 却指望读压缩文件 |

若他短期改不了链接，可让他 **暂时不压缩** 写 H5（`compression=none`），你们也能用未开 zlib 的 HDF5 读——但不建议长期这样。
