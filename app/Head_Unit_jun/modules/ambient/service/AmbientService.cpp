#include "AmbientService.h"
#include "ILedController.h"

AmbientService::AmbientService(ILedController *led, QObject *parent)
    : QObject(parent), m_led(led) {}

void AmbientService::applyColor(quint8 r, quint8 g, quint8 b)
{
    m_r = r; m_g = g; m_b = b; m_on = true;
    if (m_led) m_led->setColor(r, g, b);
    emit colorChanged(r, g, b, m_brightness);
}

void AmbientService::setBrightness(quint8 percent)
{
    m_brightness = percent;
    if (m_led) m_led->setBrightness(percent);
    if (m_on) emit colorChanged(m_r, m_g, m_b, m_brightness);
}

void AmbientService::turnOff()
{
    m_on = false;
    if (m_led) m_led->off();
    emit lightingOff();
}
