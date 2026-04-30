/**
 * @file SettingsScreen.h
 * @brief Settings tab - speed unit, about
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include "IVehicleDataProvider.h"  // GearState

class GearStateManager;
class GearPanel;
class QLabel;

class SettingsScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsScreen(GearStateManager *gearState, QWidget *parent = nullptr);

public slots:
    void updateIpcStatus(bool connected);
    void updateSpeed(float kmh);
    void updateGear(GearState gear);

signals:
    void speedUnitChanged(bool metric);   // true = km/h, false = mph

private:
    GearPanel *m_gearPanel      = nullptr;
    QLabel    *m_ipcStatusLabel = nullptr;
    QLabel    *m_speedLabel     = nullptr;
    QLabel    *m_gearLabel      = nullptr;
    bool       m_useMetric      = true;
};

#endif // SETTINGSWINDOW_H
