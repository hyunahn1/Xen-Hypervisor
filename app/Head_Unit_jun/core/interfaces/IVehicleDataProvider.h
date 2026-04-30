/**
 * @file IVehicleDataProvider.h
 * @brief Abstract interface for vehicle data (speed, gear, battery)
 * Implementations: MockVehicleDataProvider, VSomeIPClient
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef IVEHICLEDATAPROVIDER_H
#define IVEHICLEDATAPROVIDER_H

#include <QObject>
#include <QString>

/**
 * GearState: P=0, R=1, N=2, D=3
 */
enum class GearState : quint8 {
    P = 0,
    R = 1,
    N = 2,
    D = 3
};

/**
 * @class IVehicleDataProvider
 * @brief Abstract interface - replace with VSomeIPClient for production
 */
class IVehicleDataProvider : public QObject
{
    Q_OBJECT

public:
    explicit IVehicleDataProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IVehicleDataProvider() = default;

    virtual float speed() const = 0;
    virtual GearState gear() const = 0;
    virtual float batteryVoltage() const = 0;
    virtual float batteryPercent() const = 0;
    virtual bool isConnected() const = 0;

signals:
    void speedChanged(float kmh);
    void gearChanged(GearState gear);
    void batteryChanged(float voltage, float percent);
    void connectionStatusChanged(bool connected);
};

#endif // IVEHICLEDATAPROVIDER_H
