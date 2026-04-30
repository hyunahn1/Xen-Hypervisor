/**
 * @file MockVehicleDataProvider.cpp
 * @brief Stub - simulates speed/gear/battery for development
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "MockVehicleDataProvider.h"
#include <QDebug>
#include <QtGlobal>
#include <cmath>

MockVehicleDataProvider::MockVehicleDataProvider(QObject *parent)
    : IVehicleDataProvider(parent)
    , m_speed(0.0f)
    , m_gear(GearState::P)
    , m_batteryVoltage(7.8f)
    , m_batteryPercent(85.0f)
    , m_connected(true)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MockVehicleDataProvider::simulateData);
    m_timer->start(500);  // 500ms 간격 — Pi 부하 완화 시 1000으로 증가 가능
}

void MockVehicleDataProvider::simulateData()
{
    static int tick = 0;
    tick++;

    float newSpeed = 5.0f + 10.0f * std::sin(tick * 0.1f);
    if (newSpeed < 0) newSpeed = 0;

    m_speed = newSpeed;
    emit speedChanged(m_speed);

    if (tick % 20 == 0) {
        m_batteryPercent = qBound(50.0f, m_batteryPercent - 0.1f, 100.0f);
        m_batteryVoltage = 6.4f + (m_batteryPercent / 100.0f) * 2.0f;
        emit batteryChanged(m_batteryVoltage, m_batteryPercent);
    }
}
