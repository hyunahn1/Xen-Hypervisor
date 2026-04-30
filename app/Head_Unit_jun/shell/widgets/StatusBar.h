/**
 * @file StatusBar.h
 * @brief Bottom status: speed, gear, IPC status
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

class IVehicleDataProvider;
class GearStateManager;

class StatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBar(IVehicleDataProvider *vehicleData,
                      GearStateManager *gearState,
                      QWidget *parent = nullptr);

private slots:
    void updateDisplay();

private:
    IVehicleDataProvider *m_vehicleData;
    GearStateManager *m_gearState;
    QLabel *m_speedLabel;
    QLabel *m_gearLabel;
    QLabel *m_ipcLabel;
    bool m_pendingUpdate = false;
};

#endif // STATUSBAR_H
