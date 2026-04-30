#include "SettingsService.h"

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
{
    m_settings["speedUnit"] = "km/h";
}

QString SettingsService::speedUnit() const
{
    return m_settings.value("speedUnit", "km/h").toString();
}

void SettingsService::setSpeedUnit(const QString &unit)
{
    if (m_settings.value("speedUnit") == unit) return;
    m_settings["speedUnit"] = unit;
    emit settingChanged("speedUnit", unit);
}
