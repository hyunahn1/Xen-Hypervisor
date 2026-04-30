#include "PdcOverlayPainter.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
QColor colorForLevel(PdcWarningLevel level, int alpha)
{
    switch (level) {
    case PdcWarningLevel::Critical: return QColor(255, 64, 64, alpha);
    case PdcWarningLevel::Caution: return QColor(255, 210, 64, alpha);
    case PdcWarningLevel::Near: return QColor(45, 220, 120, alpha);
    case PdcWarningLevel::Far: return QColor(45, 180, 110, alpha);
    case PdcWarningLevel::Off:
    default:
        return QColor(150, 150, 150, alpha);
    }
}

PdcWarningLevel levelForDistance(float distanceCm)
{
    if (distanceCm < 0.0f) return PdcWarningLevel::Off;
    if (distanceCm < 30.0f) return PdcWarningLevel::Critical;
    if (distanceCm < 60.0f) return PdcWarningLevel::Caution;
    if (distanceCm < 120.0f) return PdcWarningLevel::Near;
    return PdcWarningLevel::Far;
}

void drawGuideLine(QPainter *p, const QRect &r, float yNorm, float topWidthNorm,
                   float bottomWidthNorm, const QColor &color, int lineWidth)
{
    const float cx = r.center().x();
    const float y = r.top() + r.height() * yNorm;
    const float lowerY = y + r.height() * 0.11f;
    const float topHalf = r.width() * topWidthNorm * 0.5f;
    const float bottomHalf = r.width() * bottomWidthNorm * 0.5f;

    QPainterPath path;
    path.moveTo(cx - topHalf, y);
    path.cubicTo(cx - topHalf * 0.94f, lowerY,
                 cx - bottomHalf * 0.94f, lowerY,
                 cx - bottomHalf, lowerY);
    path.moveTo(cx + topHalf, y);
    path.cubicTo(cx + topHalf * 0.94f, lowerY,
                 cx + bottomHalf * 0.94f, lowerY,
                 cx + bottomHalf, lowerY);
    path.moveTo(cx - bottomHalf, lowerY);
    path.lineTo(cx + bottomHalf, lowerY);

    p->setPen(QPen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
}

float intensityForDistance(float distanceCm)
{
    if (distanceCm < 0.0f) return 0.0f;
    return qBound(0.0f, (150.0f - distanceCm) / 120.0f, 1.0f);
}
}

void PdcOverlayPainter::paint(QPainter *painter, const QRect &rect, const PdcState &state)
{
    if (!painter || !state.active) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool alertVisible = !state.stale && state.warningLevel != PdcWarningLevel::Off;
    drawGuideLine(painter, rect, 0.38f, 0.26f, 0.50f,
                  QColor(45, 220, 120, alertVisible ? 130 : 70), 3);
    drawGuideLine(painter, rect, 0.54f, 0.38f, 0.68f,
                  QColor(45, 220, 120, alertVisible ? 170 : 80), 4);
    const int level = static_cast<int>(state.warningLevel);
    drawGuideLine(painter, rect, 0.68f, 0.50f, 0.84f,
                  QColor(255, 210, 64, level >= static_cast<int>(PdcWarningLevel::Caution) ? 220 : 100), 5);
    drawGuideLine(painter, rect, 0.80f, 0.64f, 0.96f,
                  QColor(255, 64, 64, state.warningLevel == PdcWarningLevel::Critical ? 240 : 100), 6);

    const int sectorCount = qMax(1, state.rearSensors.size());
    const int barY = rect.bottom() - 44;
    const int margin = 18;
    const int gap = 8;
    const int barW = (sectorCount == 1)
        ? qMin(rect.width() / 3, 220)
        : (rect.width() - margin * 2 - gap * (sectorCount - 1)) / sectorCount;
    for (int i = 0; i < sectorCount; ++i) {
        const int barX = (sectorCount == 1)
            ? rect.center().x() - barW / 2
            : rect.left() + margin + i * (barW + gap);
        QRect bar(barX, barY, barW, 20);
        QColor fill(120, 120, 120, 80);
        if (i < state.rearSensors.size() && state.rearSensors.at(i).valid && !state.stale) {
            const float distance = state.rearSensors.at(i).distanceCm;
            const int alpha = 70 + static_cast<int>(intensityForDistance(distance) * 165.0f);
            fill = colorForLevel(levelForDistance(distance), alpha);
        }

        painter->setPen(QPen(QColor(0, 0, 0, 130), 1));
        painter->setBrush(fill);
        painter->drawRoundedRect(bar, 4, 4);
    }

    QRect pill(rect.center().x() - 76, rect.bottom() - 82, 152, 28);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 145));
    painter->drawRoundedRect(pill, 6, 6);

    QFont font = painter->font();
    font.setPointSize(10);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(colorForLevel(state.warningLevel, state.stale ? 130 : 255));

    QString label = QStringLiteral("PDC --");
    if (state.stale) {
        label = QStringLiteral("PDC STALE");
    } else if (state.nearestDistanceCm >= 0.0f) {
        label = QStringLiteral("PDC %1 cm").arg(qRound(state.nearestDistanceCm));
    }
    painter->drawText(pill, Qt::AlignCenter, label);

    if (!state.fault.isEmpty()) {
        font.setPointSize(8);
        font.setBold(false);
        painter->setFont(font);
        painter->setPen(QColor(255, 210, 64, 210));
        painter->drawText(QRect(rect.left() + 12, rect.top() + 10, rect.width() - 24, 20),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          state.fault);
    }

    painter->restore();
}
