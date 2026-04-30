/**
 * @file ILedController.h
 * @brief Abstract LED interface - implementations: PwmLedController, WS2812LedController
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef ILEDCONTROLLER_H
#define ILEDCONTROLLER_H

#include <QObject>
#include <QString>

class ILedController : public QObject
{
    Q_OBJECT

public:
    explicit ILedController(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ILedController() = default;

    virtual bool init() = 0;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void setBrightness(uint8_t percent) = 0;
    virtual void setPreset(const QString &presetName) = 0;
    virtual void off() = 0;
};

#endif // ILEDCONTROLLER_H
