#include "PdcBeepController.h"

#include <QApplication>

PdcBeepController::PdcBeepController(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &PdcBeepController::beep);
}

void PdcBeepController::setPdcState(const PdcState &state)
{
    const int interval = (!state.active || state.stale)
        ? 0
        : intervalForLevel(state.warningLevel);

    if (interval <= 0) {
        m_timer.stop();
        return;
    }

    if (!m_timer.isActive() || m_timer.interval() != interval) {
        m_timer.start(interval);
    }
}

void PdcBeepController::beep()
{
    QApplication::beep();
}

int PdcBeepController::intervalForLevel(PdcWarningLevel level) const
{
    switch (level) {
    case PdcWarningLevel::Near: return 800;
    case PdcWarningLevel::Caution: return 350;
    case PdcWarningLevel::Critical: return 120;
    case PdcWarningLevel::Off:
    case PdcWarningLevel::Far:
    default:
        return 0;
    }
}
