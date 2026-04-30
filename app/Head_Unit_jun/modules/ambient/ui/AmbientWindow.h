/**
 * @file AmbientScreen.h
 * @brief Lighting tab - LED presets, color swatches, brightness control
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef AMBIENTWINDOW_H
#define AMBIENTWINDOW_H

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QLabel>

class ILedController;
class GearStateManager;
class GearPanel;

class AmbientScreen : public QWidget
{
    Q_OBJECT

public:
    explicit AmbientScreen(ILedController *ledController,
                           GearStateManager *gearState,
                           QWidget *parent = nullptr);

signals:
    // MainWindow connects this to the global GlowOverlay
    void ambientColorChanged(uint8_t r, uint8_t g, uint8_t b, int brightness);
    void ambientOff();

private slots:
    void onPresetClicked(const QString &preset);
    void onColorSwatchClicked(uint8_t r, uint8_t g, uint8_t b, const QString &name);
    void onBrightnessChanged(int value);
    void onOffClicked();

private:
    QPushButton *makePresetBtn(const QString &label, const QString &preset);
    QPushButton *makeColorBtn(const QColor &color, const QString &name);
    void        applyColor(uint8_t r, uint8_t g, uint8_t b);

    ILedController *m_ledController;
    GearPanel      *m_gearPanel;

    QLabel  *m_colorPreview;
    QLabel  *m_brightnessLabel;
    QSlider *m_brightnessSlider;

    uint8_t m_r = 255, m_g = 255, m_b = 255;
    int     m_brightness = 75;
    bool    m_colorActive = false;
};

#endif // AMBIENTWINDOW_H
