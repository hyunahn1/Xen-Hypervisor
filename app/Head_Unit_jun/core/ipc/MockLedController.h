/**
 * @file MockLedController.h
 * @brief Stub LED - no hardware, for development
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef MOCKLEDCONTROLLER_H
#define MOCKLEDCONTROLLER_H

#include "ILedController.h"

class MockLedController : public ILedController
{
    Q_OBJECT

public:
    explicit MockLedController(QObject *parent = nullptr);

    bool init() override { return true; }
    void setColor(uint8_t r, uint8_t g, uint8_t b) override;
    void setBrightness(uint8_t percent) override;
    void setPreset(const QString &presetName) override;
    void off() override;

private:
    uint8_t m_r, m_g, m_b;
    uint8_t m_brightness;
};

#endif // MOCKLEDCONTROLLER_H
