/**
 * @file GlowOverlay.h
 * @brief Full-window ambient glow overlay - top/bottom gradient from LED color
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef GLOWOVERLAY_H
#define GLOWOVERLAY_H

#include <QWidget>
#include <QColor>

/**
 * @class GlowOverlay
 * @brief Transparent overlay that draws top/bottom gradient glow matching
 *        the ambient LED color. Sits on top of all content; mouse events pass through.
 *
 * Usage:
 *   auto *glow = new GlowOverlay(centralWidget);
 *   glow->setGeometry(centralWidget->rect());
 *   glow->raise();
 *   glow->setGlow(r, g, b, brightness);  // call on color/brightness change
 *   glow->clearGlow();                   // call on Off
 */
class GlowOverlay : public QWidget
{
public:
    explicit GlowOverlay(QWidget *parent = nullptr);

    void setGlow(uint8_t r, uint8_t g, uint8_t b, int brightness);
    void clearGlow();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color;
    int    m_brightness = 75;
    bool   m_active     = false;
};

#endif // GLOWOVERLAY_H
