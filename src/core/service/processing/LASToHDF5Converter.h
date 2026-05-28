#ifndef LASTOHDF5CONVERTER_H
#define LASTOHDF5CONVERTER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QDateTime>
#include <hdf5.h>

struct LASDataPoint {
    double depth;
    double temperature;
};

class LASToHDF5Converter : public QObject
{
    Q_OBJECT

public:
    explicit LASToHDF5Converter(QObject *parent = nullptr);
    ~LASToHDF5Converter();

    bool convertLASFile(const QString& lasFilePath, const QString& outputH5Path);

private:
    bool readLASFile(const QString& filePath);
    bool createHDF5File(const QString& filePath);
    QString generateUUID() const;
    QString getCurrentISO8601Time() const;
    qint64 getMicrosecondsTimestamp(const QDateTime& dt) const;
    QString parseWellInfo(const QString& line, const QString& field) const;

    // HDF5辅助函数
    bool writeStringAttribute(hid_t loc_id, const char* attr_name, const QString& value);
    bool writeDoubleAttribute(hid_t loc_id, const char* attr_name, double value);
    bool writeIntAttribute(hid_t loc_id, const char* attr_name, int value);
    bool writeInt64Attribute(hid_t loc_id, const char* attr_name, long long value);
    bool writeHSizeAttribute(hid_t loc_id, const char* attr_name, hsize_t value);

    QVector<LASDataPoint> m_dataPoints;

    // LAS文件解析出的字段
    QString m_version;
    double m_startDepth;
    double m_stopDepth;
    double m_step;
    double m_nullValue;

    // 从LAS文件读取的元数据
    QString m_company;
    QString m_wellName;
    QString m_field;
    QString m_location;
    //QDateTime m_date;
    QString m_date;

    // 从曲线信息读取
    QString m_depthCurveName;
    QString m_tempCurveName;
    QString m_depthUnit;
    QString m_tempUnit;
};

#endif // LASTOHDF5CONVERTER_H
