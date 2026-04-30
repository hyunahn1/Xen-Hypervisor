#ifndef IPDCSENSORPROVIDER_H
#define IPDCSENSORPROVIDER_H

#include "PdcTypes.h"

#include <QObject>

class IPdcSensorProvider : public QObject
{
    Q_OBJECT

public:
    explicit IPdcSensorProvider(QObject *parent = nullptr) : QObject(parent) {}
    ~IPdcSensorProvider() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isAvailable() const = 0;

signals:
    void readingsChanged(const QVector<PdcSensorReading> &readings);
    void faultChanged(const QString &message);
};

#endif // IPDCSENSORPROVIDER_H
