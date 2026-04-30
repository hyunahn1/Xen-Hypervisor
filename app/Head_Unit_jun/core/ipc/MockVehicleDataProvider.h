/**
 * @file MockVehicleDataProvider.h
 * @brief Stub implementation for development - replace with VSomeIPClient
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef MOCKVEHICLEDATAPROVIDER_H
#define MOCKVEHICLEDATAPROVIDER_H

#include "IVehicleDataProvider.h"
#include <QTimer>

class MockVehicleDataProvider : public IVehicleDataProvider
{
    Q_OBJECT

public:
    explicit MockVehicleDataProvider(QObject *parent = nullptr);

    float speed() const override { return m_speed; }
    GearState gear() const override { return m_gear; }
    float batteryVoltage() const override { return m_batteryVoltage; }
    float batteryPercent() const override { return m_batteryPercent; }
    bool isConnected() const override { return m_connected; }

private slots:
    void simulateData();

private:
    float m_speed;
    GearState m_gear;
    float m_batteryVoltage;
    float m_batteryPercent;
    bool m_connected;
    QTimer *m_timer;
};

#endif // MOCKVEHICLEDATAPROVIDER_H
