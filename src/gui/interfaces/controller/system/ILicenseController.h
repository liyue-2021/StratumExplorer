#ifndef ILICENSECONTROLLER_H
#define ILICENSECONTROLLER_H

#include <QObject>
#include <QString>

// 许可证控制器 抽象接口
class ILicenseController : public QObject
{
    Q_OBJECT
public:
    explicit ILicenseController(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ILicenseController() = default;

    // 纯虚接口（子类必须实现）
    virtual void onRequestLoadLicense() = 0;
    virtual void onRequestValidateSave(const QString& code) = 0;

signals:
    void licenseLoaded(const QString& code);   // 许可证加载完成信号
    void saveResult(bool success);             // 保存结果信号
};

#endif // ILICENSECONTROLLER_H
