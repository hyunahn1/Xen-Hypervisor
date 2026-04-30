/**
 * @file GearStateManager.cpp
 * @brief Last-Write-Wins gear state
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "GearStateManager.h"

GearStateManager::GearStateManager(QObject *parent)
    : QObject(parent)
    , m_gear(GearState::P)
    , m_lastSource("touch")
{
}

void GearStateManager::setGearFromTouch(GearState gear)
{
    setGear(gear, "touch");
}

void GearStateManager::setGearFromThrottle(GearState gear)
{
    setGear(gear, "throttle");
}

void GearStateManager::setGearFromButton(GearState gear)
{
    setGear(gear, "button");
}

void GearStateManager::setGear(GearState gear, const QString &source)
{
    m_gear = gear;
    m_lastSource = source;
    emit gearChanged(m_gear, m_lastSource);
}
