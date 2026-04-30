/**
 * @file MockLedController.cpp
 * @brief No-op for development
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "MockLedController.h"
#include <QDebug>

MockLedController::MockLedController(QObject *parent)
    : ILedController(parent)
    , m_r(255)
    , m_g(255)
    , m_b(255)
    , m_brightness(100)
{
}

void MockLedController::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    m_r = r; m_g = g; m_b = b;
    qDebug() << "MockLed: color" << r << g << b;
}

void MockLedController::setBrightness(uint8_t percent)
{
    m_brightness = percent;
    qDebug() << "MockLed: brightness" << percent;
}

void MockLedController::setPreset(const QString &presetName)
{
    qDebug() << "MockLed: preset" << presetName;
}

void MockLedController::off()
{
    m_brightness = 0;
    qDebug() << "MockLed: off";
}
