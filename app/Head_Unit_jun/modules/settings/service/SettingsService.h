/**
 * @file SettingsService.h
 * @brief Settings 모듈 서비스 레이어 — QSettings 기반 설정 관리
 */
#ifndef SETTINGSSERVICE_H
#define SETTINGSSERVICE_H

#include <QObject>
#include <QVariantMap>

class SettingsService : public QObject
{
    Q_OBJECT
public:
    explicit SettingsService(QObject *parent = nullptr);

    QString speedUnit() const;
    void setSpeedUnit(const QString &unit);

signals:
    void settingChanged(const QString &key, const QVariant &value);

private:
    QVariantMap m_settings;
};
#endif // SETTINGSSERVICE_H
