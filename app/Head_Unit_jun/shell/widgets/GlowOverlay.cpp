/**
 * @file GlowOverlay.cpp
 * @brief Ambient glow overlay - top/bottom gradient
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "GlowOverlay.h"
#include <QPainter>
#include <QLinearGradient>

GlowOverlay::GlowOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setStyleSheet("background: transparent;");
}

void GlowOverlay::setGlow(uint8_t r, uint8_t g, uint8_t b, int brightness)
{
    m_color      = QColor(r, g, b);
    m_brightness = qBound(0, brightness, 100);
    m_active     = true;
    update();
}

void GlowOverlay::clearGlow()
{
    m_active = false;
    update();
}

void GlowOverlay::paintEvent(QPaintEvent *)
{
    if (!m_active || !m_color.isValid()) return;

    QPainter p(this);

    // Gradient height: up to 220px or half the widget height
    const int gradH = qMin(220, height() / 2);

    // Alpha scaled by brightness (max ~85 for a visible but not overwhelming glow)
    const int alpha = int(85 * m_brightness / 100.0);

    const QColor glow(m_color.red(), m_color.green(), m_color.blue(), alpha);
    const QColor none(m_color.red(), m_color.green(), m_color.blue(), 0);

    // ── Top glow ─────────────────────────────────────────────
    QLinearGradient top(0, 0, 0, gradH);
    top.setColorAt(0.0, glow);
    top.setColorAt(1.0, none);
    p.fillRect(0, 0, width(), gradH, top);

    // ── Bottom glow ──────────────────────────────────────────
    QLinearGradient bot(0, height() - gradH, 0, height());
    bot.setColorAt(0.0, none);
    bot.setColorAt(1.0, glow);
    p.fillRect(0, height() - gradH, width(), gradH, bot);
}
