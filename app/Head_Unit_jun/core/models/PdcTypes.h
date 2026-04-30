#ifndef PDCTYPES_H
#define PDCTYPES_H

#include <QMetaType>
#include <QString>
#include <QVector>

enum class PdcWarningLevel : quint8 {
    Off = 0,
    Far,
    Near,
    Caution,
    Critical
};

struct PdcSensorReading {
    QString name;
    float distanceCm = -1.0f;
    bool valid = false;
    qint64 timestampMs = 0;
};

struct PdcState {
    QVector<PdcSensorReading> rearSensors;
    float nearestDistanceCm = -1.0f;
    PdcWarningLevel warningLevel = PdcWarningLevel::Off;
    int trackedSensorIndex = -1;
    QString trackedSensorName;
    int confidencePercent = 0;
    int validSensorCount = 0;
    bool distanceHeld = false;
    bool active = false;
    bool stale = true;
    QString fault;
};

Q_DECLARE_METATYPE(PdcSensorReading)
Q_DECLARE_METATYPE(QVector<PdcSensorReading>)
Q_DECLARE_METATYPE(PdcState)

#endif // PDCTYPES_H
