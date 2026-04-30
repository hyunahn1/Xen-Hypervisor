/**
 * @file GearStateManager.h
 * @brief Gear state storage, Last-Write-Wins conflict resolution
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef GEARSTATEMANAGER_H
#define GEARSTATEMANAGER_H

#include <QObject>
#include "IVehicleDataProvider.h"

class GearStateManager : public QObject
{
    Q_OBJECT

public:
    explicit GearStateManager(QObject *parent = nullptr);

    GearState gear() const { return m_gear; }
    QString lastSource() const { return m_lastSource; }  // "throttle" | "touch" | "button"

signals:
    void gearChanged(GearState gear, const QString &source);

public slots:
    void setGearFromTouch(GearState gear);
    void setGearFromThrottle(GearState gear);
    void setGearFromButton(GearState gear);
    void setGear(GearState gear, const QString &source);

private:
    GearState m_gear;
    QString m_lastSource;
};

#endif // GEARSTATEMANAGER_H
