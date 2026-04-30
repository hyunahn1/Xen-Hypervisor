/**
 * @file AmbientService.h
 * @brief Ambient 모듈 서비스 레이어 — LED 상태 관리
 *
 * colorChanged 시그널 → ShellClient::sendAmbientColor → Shell → GlowOverlay
 */
#ifndef AMBIENTSERVICE_H
#define AMBIENTSERVICE_H

#include <QObject>
#include <cstdint>

class ILedController;

class AmbientService : public QObject
{
    Q_OBJECT
public:
    explicit AmbientService(ILedController *led, QObject *parent = nullptr);

    quint8 r()          const { return m_r; }
    quint8 g()          const { return m_g; }
    quint8 b()          const { return m_b; }
    quint8 brightness() const { return m_brightness; }
    bool   isOn()       const { return m_on; }

public slots:
    void applyColor(quint8 r, quint8 g, quint8 b);
    void setBrightness(quint8 percent);
    void turnOff();

signals:
    void colorChanged(quint8 r, quint8 g, quint8 b, quint8 brightness);
    void lightingOff();

private:
    ILedController *m_led;
    quint8 m_r = 0, m_g = 212, m_b = 170;
    quint8 m_brightness = 75;
    bool   m_on = false;
};
#endif // AMBIENTSERVICE_H
