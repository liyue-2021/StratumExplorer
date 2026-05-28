#include "LASToHDF5Converter.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QtCore>

LASToHDF5Converter::LASToHDF5Converter(QObject *parent)
    : QObject(parent)
    , m_startDepth(0.0)
    , m_stopDepth(0.0)
    , m_step(0.0)
    , m_nullValue(-999.25)
{
}

LASToHDF5Converter::~LASToHDF5Converter()
{
}

bool LASToHDF5Converter::writeStringAttribute(hid_t loc_id, const char* attr_name, const QString& value)
{
    hid_t attr_space = H5Screate(H5S_SCALAR);
    if (attr_space < 0) return false;

    hid_t attr_type = H5Tcopy(H5T_C_S1);
    if (attr_type < 0) {
        H5Sclose(attr_space);
        return false;
    }

    H5Tset_size(attr_type, value.length() + 1);
    H5Tset_strpad(attr_type, H5T_STR_NULLTERM);

    hid_t attr = H5Acreate(loc_id, attr_name, attr_type, attr_space,
                           H5P_DEFAULT, H5P_DEFAULT);
    if (attr < 0) {
        H5Tclose(attr_type);
        H5Sclose(attr_space);
        return false;
    }

    QByteArray ba = value.toUtf8();
    herr_t status = H5Awrite(attr, attr_type, ba.data());

    H5Aclose(attr);
    H5Tclose(attr_type);
    H5Sclose(attr_space);

    return status >= 0;
}

bool LASToHDF5Converter::writeDoubleAttribute(hid_t loc_id, const char* attr_name, double value)
{
    hid_t attr_space = H5Screate(H5S_SCALAR);
    if (attr_space < 0) return false;

    hid_t attr = H5Acreate(loc_id, attr_name, H5T_NATIVE_DOUBLE, attr_space,
                           H5P_DEFAULT, H5P_DEFAULT);
    if (attr < 0) {
        H5Sclose(attr_space);
        return false;
    }

    herr_t status = H5Awrite(attr, H5T_NATIVE_DOUBLE, &value);

    H5Aclose(attr);
    H5Sclose(attr_space);

    return status >= 0;
}

bool LASToHDF5Converter::writeIntAttribute(hid_t loc_id, const char* attr_name, int value)
{
    hid_t attr_space = H5Screate(H5S_SCALAR);
    if (attr_space < 0) return false;

    hid_t attr = H5Acreate(loc_id, attr_name, H5T_NATIVE_INT, attr_space,
                           H5P_DEFAULT, H5P_DEFAULT);
    if (attr < 0) {
        H5Sclose(attr_space);
        return false;
    }

    herr_t status = H5Awrite(attr, H5T_NATIVE_INT, &value);

    H5Aclose(attr);
    H5Sclose(attr_space);

    return status >= 0;
}

bool LASToHDF5Converter::writeInt64Attribute(hid_t loc_id, const char* attr_name, long long value)
{
    hid_t attr_space = H5Screate(H5S_SCALAR);
    if (attr_space < 0) return false;

    hid_t attr = H5Acreate(loc_id, attr_name, H5T_NATIVE_LLONG, attr_space,
                           H5P_DEFAULT, H5P_DEFAULT);
    if (attr < 0) {
        H5Sclose(attr_space);
        return false;
    }

    herr_t status = H5Awrite(attr, H5T_NATIVE_LLONG, &value);

    H5Aclose(attr);
    H5Sclose(attr_space);

    return status >= 0;
}

bool LASToHDF5Converter::writeHSizeAttribute(hid_t loc_id, const char* attr_name, hsize_t value)
{
    hid_t attr_space = H5Screate(H5S_SCALAR);
    if (attr_space < 0) return false;

    hid_t attr = H5Acreate(loc_id, attr_name, H5T_NATIVE_HSIZE, attr_space,
                           H5P_DEFAULT, H5P_DEFAULT);
    if (attr < 0) {
        H5Sclose(attr_space);
        return false;
    }

    herr_t status = H5Awrite(attr, H5T_NATIVE_HSIZE, &value);

    H5Aclose(attr);
    H5Sclose(attr_space);

    return status >= 0;
}

QString LASToHDF5Converter::parseWellInfo(const QString& line, const QString& field) const
{
    if (line.startsWith(field)) {
        int colonPos = line.indexOf(":");
        if (colonPos != -1) {
            QString value = line.mid(colonPos + 1).trimmed();
            return value;
        }
    }
    return QString();
}

bool LASToHDF5Converter::readLASFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open LAS file:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    bool inASCII = false;
    QString section;

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.isEmpty()) continue;

        if (line.startsWith("~")) {
            inASCII = false;
            section = line;
            continue;
        }

        if (section.startsWith("~Version")) {
            if (line.contains("VERS.")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    m_version = parts[1];
                }
            }
        }
        else if (section.startsWith("~Well")) {
#if 0
            QString comp = parseWellInfo(line, "COMP.");
            if (!comp.isEmpty()) m_company = comp;

            QString well = parseWellInfo(line, "WELL.");
            if (!well.isEmpty()) m_wellName = well;

            QString fld = parseWellInfo(line, "FLD.");
            if (!fld.isEmpty()) m_field = fld;

            QString loc = parseWellInfo(line, "LOC.");
            if (!loc.isEmpty()) m_location = loc;

            QString dateStr = parseWellInfo(line, "DATE.");
            if (!dateStr.isEmpty()) {
                dateStr.replace('.', ':');
                QDateTime dt = QDateTime::fromString(dateStr, "yyyy/MM/dd hh:mm:ss");
                if (dt.isValid()) {
                    m_date = dt;
                    qDebug() << "Found acquisition date:" << m_date;
                }
            }
#endif

            if (line.contains("STRT.ft")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_startDepth = parts[1].toDouble();
                }
            }
            else if (line.contains("STOP.ft")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_stopDepth = parts[1].toDouble();
                }
            }
            else if (line.contains("STEP.ft")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_step = parts[1].toDouble();
                }
            }
            else if (line.contains("NULL.")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_nullValue = parts[1].toDouble();
                }
            }
            else if (line.contains("COMP.")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_company = parts[1];
                }
            }
            else if (line.contains("WELL.")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    m_wellName = parts[1];
                }
            }
            else if (line.contains("FLD .")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                qDebug() << "FLD size: " << parts.size() << ", parts: " << parts;
                if (parts.size() >= 3 && parts[2] != ":") {
                    m_field = parts[2];
                }
            }
            else if (line.contains("LOC .")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 3 && parts[2] != ":") {
                    m_location = parts[2];
                }
            }
            else if (line.contains("DATE.")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2 && parts[1] != ":") {
                    QString dateStr = parts[1] + " " + parts[2];
                    if (!dateStr.isEmpty()) {
                        dateStr.replace('.', ':');
                        QDateTime dt = QDateTime::fromString(dateStr, "yyyy/MM/dd hh:mm:ss");
                        if (dt.isValid()) {
                            m_date = dt.toString("yyyy-MM-ddThh:mm:ss.zzz") + "000Z";
                            qDebug() << "Found acquisition date:" << m_date;
                        }
                    }
                }
#if 0
                int colonPos = line.indexOf(":");
                if (colonPos != -1) {
                    QString dateStr = line.mid(colonPos + 1).trimmed();
                    qDebug() << "m_date: " << dateStr;
                    // 格式: 2025/07/04 11.44.47
                    dateStr.replace('.', ':'); // 将时间中的点替换为冒号
                    QDateTime dt = QDateTime::fromString(dateStr, "yyyy/MM/dd hh:mm:ss");
                    if (dt.isValid()) {
                        m_date = dt;
                        qDebug() << "Found acquisition date:" << m_date;
                    }
                }
#endif
            }
        }
        else if (section.startsWith("~Curve")) {
            if (line.contains("DEPTH")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 3) {
                    m_depthCurveName = parts[0];
                    m_depthUnit = parts[1];
                }
            }
            else if (line.contains("TEMP")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 3) {
                    m_tempCurveName = parts[0];
                    m_tempUnit = parts[1];
                }
            }
        }
        else if (section.startsWith("~ASCII")) {
            inASCII = true;
            QStringList values = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (values.size() >= 2) {
                LASDataPoint point;
                point.depth = values[0].toDouble();
                point.temperature = values[1].toDouble();

                if (point.temperature != m_nullValue) {
                    m_dataPoints.append(point);
                }
            }
        }
    }

    file.close();

    if (m_dataPoints.isEmpty()) {
        qCritical() << "No data points found in LAS file";
        return false;
    }

    qDebug() << "========================================";
    qDebug() << "LAS File Information:";
    qDebug() << "  Well Name:" << (m_wellName.isEmpty() ? "(empty)" : m_wellName);
    qDebug() << "  Company:" << (m_company.isEmpty() ? "(empty)" : m_company);
    qDebug() << "  Field:" << (m_field.isEmpty() ? "(empty)" : m_field);
    qDebug() << "  Location:" << (m_location.isEmpty() ? "(empty)" : m_location);
    qDebug() << "  Date:" << (m_date.isEmpty() ?  "(empty)" : m_date);
    qDebug() << "  Start Depth:" << m_startDepth << "ft";
    qDebug() << "  Stop Depth:" << m_stopDepth << "ft";
    qDebug() << "  Step:" << m_step << "ft";
    qDebug() << "  Data Points:" << m_dataPoints.size();
    qDebug() << "========================================";

    return true;
}

QString LASToHDF5Converter::generateUUID() const
{
    QUuid uuid = QUuid::createUuid();
    return uuid.toString(QUuid::WithoutBraces);
}

QString LASToHDF5Converter::getCurrentISO8601Time() const
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) + "Z";
}

qint64 LASToHDF5Converter::getMicrosecondsTimestamp(const QDateTime& dt) const
{
    if (dt.isValid()) {
        return dt.toMSecsSinceEpoch() * 1000;
    }
    return 0;
}

bool LASToHDF5Converter::createHDF5File(const QString& filePath)
{
    hid_t file_id = H5Fcreate(filePath.toStdString().c_str(), H5F_ACC_TRUNC,
                              H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        qCritical() << "Failed to create HDF5 file:" << filePath;
        return false;
    }

    // ========== Acquisition Group ==========
    hid_t acquisition_grp = H5Gcreate(file_id, "/Acquisition", H5P_DEFAULT,
                                      H5P_DEFAULT, H5P_DEFAULT);
    if (acquisition_grp < 0) {
        H5Fclose(file_id);
        return false;
    }

    writeStringAttribute(acquisition_grp, "Version", m_version);

    QString dataUuid = generateUUID();
    writeStringAttribute(acquisition_grp, "DataUuid", dataUuid);

    // 如何读取项目名？
    writeStringAttribute(acquisition_grp, "ProjectName", "");  // 置空

    writeStringAttribute(acquisition_grp, "WellName", m_wellName);

    // 是否是停止的深度？
    writeDoubleAttribute(acquisition_grp, "TotalWellDepth", m_stopDepth);

    writeStringAttribute(acquisition_grp, "DataType", "Raw");

    QString createTime = getCurrentISO8601Time();
    writeStringAttribute(acquisition_grp, "CreateTime", createTime);

    // ========== Raw Group ==========
    hid_t raw_grp = H5Gcreate(file_id, "/Raw", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (raw_grp < 0) {
        H5Gclose(acquisition_grp);
        H5Fclose(file_id);
        return false;
    }

    QString acquisitionTaskUuid = generateUUID();
    writeStringAttribute(raw_grp, "AcquisitionTaskUuid", acquisitionTaskUuid);

    // 如何确定DeviceType?
    writeStringAttribute(raw_grp, "DeviceType", "DTS");

    // 如何确定ChannelNo?默认值填什么？
    int channelNo = 1;
    writeIntAttribute(raw_grp, "ChannelNo", channelNo);

    // 是否是起始深度？
    writeDoubleAttribute(raw_grp, "FiberStart", m_startDepth);

    // 起始索引点是否一定为0
    int locusStartIndex = 0;
    writeIntAttribute(raw_grp, "LocusStartIndex", locusStartIndex);

    hsize_t dataChannelNum = m_dataPoints.size();
    writeHSizeAttribute(raw_grp, "DataChannelNum", dataChannelNum);

    // 和起始光纤长度FiberStart有什么区别？
    writeDoubleAttribute(raw_grp, "WellStartDepth", m_startDepth);

    writeDoubleAttribute(raw_grp, "SpatialRes", m_step);

    // 如何确定采样率？
    double sampleRate = 1.0;
    writeDoubleAttribute(raw_grp, "SampleRate", sampleRate);

    QString unit = m_tempUnit.isEmpty() ? "degF" : m_tempUnit;
    writeStringAttribute(raw_grp, "Unit", unit);

    // ========== RawData Dataset ==========
    hsize_t dims[2] = {1, dataChannelNum};
    hid_t dataspace = H5Screate_simple(2, dims, NULL);
    hid_t dataset = H5Dcreate(raw_grp, "RawData", H5T_NATIVE_DOUBLE, dataspace,
                              H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    QVector<double> temperatures;
    for (const auto& point : m_dataPoints) {
        temperatures.append(point.temperature);
    }

    herr_t status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                             H5P_DEFAULT, temperatures.data());

    // RawData属性
    writeHSizeAttribute(dataset, "Count", dataChannelNum);

    // 开始时间是否对应源文件中的DATE字段？
    QString partStartTime = getCurrentISO8601Time();
    writeStringAttribute(dataset, "PartStartTime", partStartTime);
    // 结束时间从哪里获取？
    writeStringAttribute(dataset, "PartEndTime", partStartTime);

    int startIndex = 0;
    writeIntAttribute(dataset, "StartIndex", startIndex);

    hsize_t timeDim = 1;
    hsize_t locusDim = dataChannelNum;
    writeHSizeAttribute(dataset, "Dimensions_Time", timeDim);
    writeHSizeAttribute(dataset, "Dimensions_Locus", locusDim);

    H5Dclose(dataset);
    H5Sclose(dataspace);

    // ========== RawDataTime Dataset ==========
    dims[0] = 1;
    dataspace = H5Screate_simple(1, dims, NULL);
    dataset = H5Dcreate(raw_grp, "RawDataTime", H5T_NATIVE_UINT64, dataspace,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    //qint64 timestamp = getMicrosecondsTimestamp(m_date);
    const char* cTimeStr = m_date.toStdString().c_str();
    status = H5Dwrite(dataset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL,
                      H5P_DEFAULT, &cTimeStr);

    // RawDataTime属性
    writeHSizeAttribute(dataset, "Count", dataChannelNum);
    writeIntAttribute(dataset, "StartIndex", startIndex);
    writeStringAttribute(dataset, "PartStartTime", partStartTime);
    writeStringAttribute(dataset, "PartEndTime", partStartTime);

    H5Dclose(dataset);
    H5Sclose(dataspace);

    // 源数据是否没有Processed组？
#if 0
    // ========== Processed Group ==========
    hid_t processed_grp = H5Gcreate(file_id, "/Processed", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(processed_grp);
#endif

    H5Gclose(raw_grp);
    H5Gclose(acquisition_grp);
    H5Fclose(file_id);

    qDebug() << "========================================";
    qDebug() << "HDF5 file created successfully:" << filePath;
    qDebug() << "Version:" << m_version;
    qDebug() << "Data UUID:" << dataUuid;
    qDebug() << "Acquisition Task UUID:" << acquisitionTaskUuid;
    qDebug() << "Timestamp:" << m_date;
    qDebug() << "Well Name:" << (m_wellName.isEmpty() ? "(empty)" : m_wellName);
    qDebug() << "Total Well Depth:" << m_stopDepth << "ft";
    qDebug() << "Data points:" << dataChannelNum;
    qDebug() << "Dimensions: 1 time point x" << dataChannelNum << "spatial points";
    qDebug() << "========================================";

    return true;
}

bool LASToHDF5Converter::convertLASFile(const QString& lasFilePath, const QString& outputH5Path)
{
    qDebug() << "Converting LAS file:" << lasFilePath;

    if (!readLASFile(lasFilePath)) {
        return false;
    }

    return createHDF5File(outputH5Path);
}