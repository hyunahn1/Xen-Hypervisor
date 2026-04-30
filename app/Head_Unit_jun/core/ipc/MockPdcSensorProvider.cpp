#include "MockPdcSensorProvider.h"

#include <QDateTime>
#include <QtMath>

namespace {
const QString kSensorName = QStringLiteral("rear_center");
}

MockPdcSensorProvider::MockPdcSensorProvider(QObject *parent)
    : IPdcSensorProvider(parent)
{
    m_timer.setInterval(120);
    connect(&m_timer, &QTimer::timeout,
            this, &MockPdcSensorProvider::publishNextFrame);
}

void MockPdcSensorProvider::start()
{
    if (!m_timer.isActive()) {
        m_timer.start();
        publishNextFrame();
    }
}

void MockPdcSensorProvider::stop()
{
    m_timer.stop();
}

void MockPdcSensorProvider::publishNextFrame()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const float wave = (qSin(m_phase / 18.0) + 1.0) * 0.5;
    const float nearest = 22.0f + static_cast<float>(wave) * 135.0f;

    QVector<PdcSensorReading> readings;
    readings.reserve(1);

    PdcSensorReading reading;
    reading.name = kSensorName;
    reading.distanceCm = nearest;
    reading.valid = true;
    reading.timestampMs = now;
    readings.push_back(reading);

    ++m_phase;
    emit readingsChanged(readings);
}
