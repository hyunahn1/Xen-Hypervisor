/**
 * @file StatusBar.cpp
 * @brief Speed, gear, IPC status display
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "StatusBar.h"
#include "IVehicleDataProvider.h"
#include "GearStateManager.h"
#include <QTimer>

static QString gearToString(GearState g) {
    switch (g) { case GearState::P: return "P"; case GearState::R: return "R";
    case GearState::N: return "N"; case GearState::D: return "D"; }
    return "?";
}

StatusBar::StatusBar(IVehicleDataProvider *vehicleData, GearStateManager *gearState, QWidget *parent)
    : QWidget(parent)
    , m_vehicleData(vehicleData)
    , m_gearState(gearState)
{
    setStyleSheet("background-color: #111114; color: #B3B3B7; font-size: 10pt;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 4, 10, 4);
    layout->setSpacing(24);

    m_speedLabel = new QLabel(this);
    m_gearLabel = new QLabel(this);
    m_ipcLabel = new QLabel(this);

    layout->addWidget(m_speedLabel);
    layout->addWidget(m_gearLabel);
    layout->addWidget(m_ipcLabel);
    layout->addStretch();

    connect(m_vehicleData, &IVehicleDataProvider::speedChanged, this, &StatusBar::updateDisplay);
    connect(m_vehicleData, &IVehicleDataProvider::connectionStatusChanged, this, &StatusBar::updateDisplay);
    connect(m_gearState, &GearStateManager::gearChanged, this, &StatusBar::updateDisplay);

    QTimer::singleShot(0, this, &StatusBar::updateDisplay);
}

void StatusBar::updateDisplay()
{
    if (m_vehicleData->isConnected()) {
        m_speedLabel->setText(QString("◉ %1 km/h").arg(m_vehicleData->speed(), 0, 'f', 1));
        m_speedLabel->setStyleSheet("color: #34C759;");
    } else {
        m_speedLabel->setText("✕ --- km/h");
        m_speedLabel->setStyleSheet("color: #FF4757;");
    }

    m_gearLabel->setText(QString("[Gear: %1]").arg(gearToString(m_gearState->gear())));

    m_ipcLabel->setText(m_vehicleData->isConnected() ? "◉ IPC Connected" : "✕ IPC Unavailable");
    m_ipcLabel->setStyleSheet(m_vehicleData->isConnected() ? "color: #34C759;" : "color: #FF4757;");
}
