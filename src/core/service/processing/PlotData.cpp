#include "PlotData.h"

#include <QFile>
#include <QObject>

#ifdef WITH_HDF5
#include <hdf5.h>
#include <H5Zpublic.h>
#endif

PlotData::PlotData(QObject* parent)
    : QObject(parent)
{
}

PlotData::~PlotData() = default;

void PlotData::LoadPlotData(const PlotRequest& req, PlotDisplayData* out, QString* error)
{
    if (!out) {
        if (error)
            *error = QStringLiteral("PlotDisplayData is null");
        return;
    }
    out->clear();

#ifndef WITH_HDF5
    Q_UNUSED(req);
    if (error)
        *error = QStringLiteral("HDF5 support not built (configure with -DWITH_HDF5=ON)");
    return;
#else
    if (!QFile::exists(req.h5Path)) {
        if (error)
            *error = QObject::tr("文件不存在: %1").arg(req.h5Path);
        return;
    }

    const hid_t file_id =
        H5Fopen(req.h5Path.toUtf8().constData(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        if (error)
            *error = QObject::tr("无法打开 HDF5: %1").arg(req.h5Path);
        return;
    }

    const char* datasetPath = "/Processed/Data";
    hid_t       dataset_id  = H5Dopen2(file_id, datasetPath, H5P_DEFAULT);
    if (dataset_id < 0) {
        H5Fclose(file_id);
        if (error)
            *error = QObject::tr("未找到数据集: %1").arg(QString::fromLatin1(datasetPath));
        return;
    }

    hid_t space_id = H5Dget_space(dataset_id);
    if (space_id < 0) {
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        if (error)
            *error = QObject::tr("无法读取数据空间");
        return;
    }

    const int rank = H5Sget_simple_extent_ndims(space_id);
    if (rank != 2) {
        H5Sclose(space_id);
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        if (error)
            *error = QObject::tr("Processed/Data 不是二维数据集 (rank=%1)").arg(rank);
        return;
    }

    hsize_t dims[2] = {0, 0};
    H5Sget_simple_extent_dims(space_id, dims, nullptr);
    const hsize_t rows = dims[0];
    const hsize_t cols = dims[1];

    auto fail = [&](const QString& msg) {
        if (error)
            *error = msg;
        H5Sclose(space_id);
        H5Dclose(dataset_id);
        H5Fclose(file_id);
    };

    if (req.kind == QStringLiteral("Overview")) {
        std::vector<float> buffer(static_cast<size_t>(rows) * static_cast<size_t>(cols));
        const herr_t status =
            H5Dread(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
        if (status < 0) {
            fail(QObject::tr("读取 Processed/Data 失败（若为 GZIP 压缩，请确认已链接 zlib）"));
            return;
        }

        out->rows = static_cast<int>(rows);
        out->cols = static_cast<int>(cols);
        out->data = std::move(buffer);
        out->x.resize(out->cols);
        out->y.resize(out->rows);
        for (int i = 0; i < out->cols; ++i)
            out->x[static_cast<size_t>(i)] = i;
        for (int j = 0; j < out->rows; ++j)
            out->y[static_cast<size_t>(j)] = j;
    } else if (req.kind == QStringLiteral("Channel")) {
        if (req.channelIndex < 0 || req.channelIndex >= static_cast<int>(rows)) {
            fail(QObject::tr("通道编号越界: %1 (0..%2)")
                     .arg(req.channelIndex)
                     .arg(static_cast<int>(rows) - 1));
            return;
        }

        const hsize_t offset[2] = {static_cast<hsize_t>(req.channelIndex), 0};
        const hsize_t count[2]  = {1, cols};
        if (H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, nullptr, count, nullptr) < 0) {
            fail(QObject::tr("通道 hyperslab 选择失败"));
            return;
        }

        const hsize_t memDims[2] = {1, cols};
        hid_t         memspace_id = H5Screate_simple(2, memDims, nullptr);
        std::vector<float> buffer(static_cast<size_t>(cols));
        const herr_t status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace_id, space_id,
                                      H5P_DEFAULT, buffer.data());
        H5Sclose(memspace_id);
        if (status < 0) {
            fail(QObject::tr("读取单通道数据失败"));
            return;
        }

        out->rows = 1;
        out->cols = static_cast<int>(cols);
        out->data = std::move(buffer);
        out->x.resize(out->cols);
        out->y = {req.channelIndex};
        for (int i = 0; i < out->cols; ++i)
            out->x[static_cast<size_t>(i)] = i;
    } else if (req.kind == QStringLiteral("Window")) {
        int t0 = qBound(0, req.t_start, static_cast<int>(cols) - 1);
        int t1 = qBound(0, req.t_end, static_cast<int>(cols) - 1);
        if (t0 > t1)
            std::swap(t0, t1);
        int ch0 = qBound(0, req.ch_start, static_cast<int>(rows) - 1);
        int ch1 = qBound(0, req.ch_end, static_cast<int>(rows) - 1);
        if (ch0 > ch1)
            std::swap(ch0, ch1);

        const hsize_t rowCount = static_cast<hsize_t>(ch1 - ch0 + 1);
        const hsize_t colCount = static_cast<hsize_t>(t1 - t0 + 1);
        const hsize_t offset[2] = {static_cast<hsize_t>(ch0), static_cast<hsize_t>(t0)};
        const hsize_t count[2]  = {rowCount, colCount};
        if (H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, nullptr, count, nullptr) < 0) {
            fail(QObject::tr("Window hyperslab 选择失败"));
            return;
        }

        const hsize_t memDims[2] = {rowCount, colCount};
        hid_t         memspace_id = H5Screate_simple(2, memDims, nullptr);
        std::vector<float> buffer(static_cast<size_t>(rowCount) * colCount);
        const herr_t status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace_id, space_id,
                                      H5P_DEFAULT, buffer.data());
        H5Sclose(memspace_id);
        if (status < 0) {
            fail(QObject::tr("读取 Window 数据失败"));
            return;
        }

        out->rows = static_cast<int>(rowCount);
        out->cols = static_cast<int>(colCount);
        out->data = std::move(buffer);
        out->x.resize(out->cols);
        out->y.resize(out->rows);
        for (int i = 0; i < out->cols; ++i)
            out->x[static_cast<size_t>(i)] = t0 + i;
        for (int j = 0; j < out->rows; ++j)
            out->y[static_cast<size_t>(j)] = ch0 + j;
    } else {
        fail(QObject::tr("未知 kind: %1").arg(req.kind));
        return;
    }

    H5Sclose(space_id);
    H5Dclose(dataset_id);
    H5Fclose(file_id);
#endif
}
