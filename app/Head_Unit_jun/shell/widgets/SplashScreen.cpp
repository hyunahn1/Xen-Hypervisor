/**
 * @file SplashScreen.cpp
 * @brief Car-style boot splash animation
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "SplashScreen.h"

#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QFont>

// ── Phase durations (ms) ─────────────────────────────────────────
static constexpr int kDuration[] = { 700, 1600, 550 };

SplashScreen::SplashScreen(QWidget *parent)
    : QWidget(parent)
{
    // Transparent bg so the parent shows through during fade-out
    setAttribute(Qt::WA_TranslucentBackground);
    runPhase(0);
}

void SplashScreen::runPhase(int phase)
{
    m_phase = phase;
    m_t     = 0.0f;

    auto *anim = new QVariantAnimation(this);
    anim->setStartValue(0.0f);
    anim->setEndValue(1.0f);
    anim->setDuration(kDuration[phase]);

    if (phase == 0) anim->setEasingCurve(QEasingCurve::OutQuart);  // snappy logo reveal
    if (phase == 2) anim->setEasingCurve(QEasingCurve::InCubic);   // accelerating fade-out

    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_t = v.toFloat();
        update();
    });
    connect(anim, &QVariantAnimation::finished, this, [this, phase] {
        if (phase < 2) runPhase(phase + 1);
        else           emit finished();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SplashScreen::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF r  = QRectF(rect());
    const qreal  cx = r.width()  / 2.0;
    const qreal  cy = r.height() / 2.0;

    // ── Global opacity: 1.0 during phases 0-1, falls to 0 in phase 2
    const qreal globalA = (m_phase == 2) ? qreal(1.0 - m_t) : 1.0;

    // ── Solid black background (becomes transparent during fade-out)
    p.setOpacity(globalA);
    p.fillRect(r.toRect(), QColor(0, 0, 0));

    if (globalA < 0.01) return;

    // ── Logo elements fade in during phase 0, then stay at 1.0
    const qreal logoA  = (m_phase == 0) ? qreal(m_t) : 1.0;
    // Slight upward drift while fading in
    const qreal yDrift = (m_phase == 0) ? 18.0 * (1.0 - m_t) : 0.0;

    p.setOpacity(globalA * logoA);

    // ── Circular logo badge ───────────────────────────────────────
    const qreal  circR = 52.0;
    const QPointF ctr(cx, cy - 72.0 + yDrift);

    QRadialGradient radGrad(ctr, circR);
    radGrad.setColorAt(0.0, QColor(0, 212, 170, 210));
    radGrad.setColorAt(0.6, QColor(0, 160, 130, 120));
    radGrad.setColorAt(1.0, QColor(0,  90,  80,  40));

    p.setPen(QPen(QColor(0, 212, 170, 160), 1.5));
    p.setBrush(radGrad);
    p.drawEllipse(ctr, circR, circR);

    // "P" glyph inside the badge
    {
        QFont f;
        f.setPointSize(34);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        QRectF cr(ctr.x() - circR, ctr.y() - circR, circR * 2.0, circR * 2.0);
        p.drawText(cr, Qt::AlignCenter, "P");
    }

    // ── "PiRacer" brand name ──────────────────────────────────────
    {
        QFont f;
        f.setPointSize(30);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 3);
        p.setFont(f);
        p.setPen(Qt::white);
        QRectF br(cx - 220.0, ctr.y() + circR + 18.0 + yDrift, 440.0, 52.0);
        p.drawText(br, Qt::AlignCenter, "PiRacer");
    }

    // ── "HEAD UNIT" subtitle ─────────────────────────────────────
    {
        QFont f;
        f.setPointSize(10);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 7);
        p.setFont(f);
        p.setPen(QColor(150, 150, 156));
        QRectF sr(cx - 200.0, ctr.y() + circR + 76.0 + yDrift, 400.0, 28.0);
        p.drawText(sr, Qt::AlignCenter, "HEAD UNIT");
    }

    // ── Teal separator line ───────────────────────────────────────
    p.setPen(QPen(QColor(0, 212, 170, 80), 1));
    const qreal sepY = ctr.y() + circR + 113.0 + yDrift;
    p.drawLine(QPointF(cx - 72.0, sepY), QPointF(cx + 72.0, sepY));

    // ── Progress bar (visible from phase 1 onward) ────────────────
    const qreal barFill = (m_phase == 0) ? 0.0
                        : (m_phase == 1) ? qreal(m_t)
                        : 1.0;

    p.setOpacity(globalA);

    const qreal barW = 420.0, barH = 3.0;
    const qreal barX = cx - barW / 2.0;
    const qreal barY = r.height() - 76.0;

    // Track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(35, 35, 42));
    p.drawRoundedRect(QRectF(barX, barY, barW, barH), 1.5, 1.5);

    // Fill
    if (barFill > 0.001) {
        const qreal fw = barW * barFill;

        QLinearGradient lg(barX, barY, barX + fw, barY);
        lg.setColorAt(0.0, QColor(0, 160, 130));
        lg.setColorAt(1.0, QColor(0, 212, 170));
        p.setBrush(lg);
        p.drawRoundedRect(QRectF(barX, barY, fw, barH), 1.5, 1.5);

        // Glowing leading edge
        QRadialGradient glow(barX + fw, barY + barH / 2.0, 14.0);
        glow.setColorAt(0.0, QColor(0, 212, 170, 150));
        glow.setColorAt(1.0, QColor(0, 212, 170,   0));
        p.setBrush(glow);
        p.drawEllipse(QPointF(barX + fw, barY + barH / 2.0), 14.0, 14.0);
    }

    // Status text row
    if (m_phase >= 1) {
        QFont f;
        f.setPointSize(9);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        p.setFont(f);
        p.setPen(QColor(80, 80, 88));

        const qreal textY = barY + barH + 10.0;
        p.drawText(QRectF(barX, textY, barW * 0.6, 20.0),
                   Qt::AlignLeft, "SYSTEM INITIALIZING...");
        p.drawText(QRectF(barX + barW * 0.4, textY, barW * 0.6, 20.0),
                   Qt::AlignRight, QString::number(int(barFill * 100)) + "%");
    }
}
