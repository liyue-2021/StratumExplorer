#ifndef DEVICETYPE_H
#define DEVICETYPE_H

#include <QString>
#include <QMetaEnum>

class DeviceType {
public:
    enum Value {
        Unknown = 0,
        DSS,
        DTS,
        DAS
    };
    
    static QString toString(Value type) {
        switch (type) {
            case DSS: return "DSS";
            case DTS: return "DTS";
            case DAS: return "DAS";
            default: return "Unknown";
        }
    }
    
    static Value fromString(const QString& str) {
        if (str == "DSS") return DSS;
        if (str == "DTS") return DTS;
        if (str == "DAS") return DAS;
        return Unknown;
    }
};

#endif // DEVICETYPE_H