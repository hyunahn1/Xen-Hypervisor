#include "PdcController.h"

#include "IPdcSensorProvider.h"

#include <QDebug>
#include <QtMath>

#include <algorithm>

namespace {
float medianDistance(QVector<float> samples)
{
    if (samples.isEmpty()) {
        return -1.0f;
    }

    std::sort(samples.begin(), samples.end());
    return samples.at(samples.size() / 2);
}
}

PdcController::PdcController(IPdcSensorProvider *provider, QObject *parent)
    : QObject(parent)
    , m_provider(provider)
{
    qRegisterMetaType<PdcState>("PdcState");
    qRegisterMetaType<QVector<PdcSensorReading>>("QVector<PdcSensorReading>");

    m_staleTimer.setSingleShot(true);
    m_staleTimer.setInterval(kStaleTimeoutMs);
    connect(&m_staleTimer, &QTimer::timeout, this, &PdcController::markStale);

    if (m_provider) {
        m_provider->setParent(this);
        connect(m_provider, &IPdcSensorProvider::readingsChanged,
                this, &PdcController::onReadingsChanged);
        connect(m_provider, &IPdcSensorProvider::faultChanged,
                this, &PdcController::onProviderFault);
        m_provider->start();
    }
}

void PdcController::setActive(bool active)
{
    if (m_state.active == active) {
        return;
    }

    m_state.active = active;
    if (!active) {
        m_state.warningLevel = PdcWarningLevel::Off;
        m_state.nearestDistanceCm = -1.0f;
        updateStateMetadata(RiskCandidate(), false);
        resetDistanceFilter();
    }
    publishState();
}

void PdcController::setVehicleSpeed(float kmh)
{
    m_vehicleSpeedKmh = kmh;
}

void PdcController::onReadingsChanged(const QVector<PdcSensorReading> &readings)
{
    m_state.rearSensors = readings;
    m_state.stale = false;
    m_state.fault.clear();
    m_state.validSensorCount = 0;
    for (const PdcSensorReading &reading : readings) {
        if (reading.valid && reading.distanceCm >= 0.0f) {
            ++m_state.validSensorCount;
        }
    }

    updateSensorTracks(readings);
    const RiskCandidate nearest = nearestConfirmedCandidate();

    if (nearest.sensorIndex < 0) {
        ++m_missingSampleFrames;
        if (m_displayDistanceCm >= 0.0f && m_missingSampleFrames <= kMaxMissingSampleFrames) {
            m_state.nearestDistanceCm = quantizeDistance(m_displayDistanceCm);
            const bool speedAllowsAlert = (m_vehicleSpeedKmh <= kMaxActiveSpeedKmh);
            m_state.warningLevel = (m_state.active && speedAllowsAlert)
                ? levelForDistance(m_displayDistanceCm, m_state.warningLevel)
                : PdcWarningLevel::Off;
            updateStateMetadata(RiskCandidate(), true);
        } else {
            m_displayDistanceCm = kInvalidFilteredDistance;
            m_state.nearestDistanceCm = -1.0f;
            m_state.warningLevel = PdcWarningLevel::Off;
            updateStateMetadata(RiskCandidate(), false);
        }
    } else {
        m_missingSampleFrames = 0;
        const float displayDistance = updateDisplayDistance(nearest.distanceCm);
        m_state.nearestDistanceCm = quantizeDistance(displayDistance);
        const bool speedAllowsAlert = (m_vehicleSpeedKmh <= kMaxActiveSpeedKmh);
        m_state.warningLevel = (m_state.active && speedAllowsAlert)
            ? levelForDistance(displayDistance, m_state.warningLevel)
            : PdcWarningLevel::Off;
        updateStateMetadata(nearest, false);
    }

    m_staleTimer.start();
    publishState();
}

void PdcController::onProviderFault(const QString &message)
{
    qWarning() << "[PDC]" << message;
    m_state.fault = message;
    markStale();
}

void PdcController::markStale()
{
    m_state.stale = true;
    m_state.nearestDistanceCm = -1.0f;
    m_state.warningLevel = PdcWarningLevel::Off;
    m_state.validSensorCount = 0;
    updateStateMetadata(RiskCandidate(), false);
    resetDistanceFilter();
    publishState();
}

PdcWarningLevel PdcController::levelForDistance(float distanceCm, PdcWarningLevel previousLevel) const
{
    if (distanceCm < 0.0f) return PdcWarningLevel::Off;

    switch (previousLevel) {
    case PdcWarningLevel::Critical:
        if (distanceCm < 30.0f + kWarningHysteresisCm) return PdcWarningLevel::Critical;
        break;
    case PdcWarningLevel::Caution:
        if (distanceCm < 30.0f) return PdcWarningLevel::Critical;
        if (distanceCm < 60.0f + kWarningHysteresisCm) return PdcWarningLevel::Caution;
        break;
    case PdcWarningLevel::Near:
        if (distanceCm < 60.0f - kWarningHysteresisCm) return PdcWarningLevel::Caution;
        if (distanceCm < 120.0f + kWarningHysteresisCm) return PdcWarningLevel::Near;
        break;
    case PdcWarningLevel::Far:
        if (distanceCm >= 120.0f - kWarningHysteresisCm) return PdcWarningLevel::Far;
        break;
    case PdcWarningLevel::Off:
    default:
        break;
    }

    if (distanceCm < 30.0f) return PdcWarningLevel::Critical;
    if (distanceCm < 60.0f) return PdcWarningLevel::Caution;
    if (distanceCm < 120.0f) return PdcWarningLevel::Near;
    return PdcWarningLevel::Far;
}

void PdcController::updateSensorTracks(const QVector<PdcSensorReading> &readings)
{
    if (m_sensorTracks.size() != readings.size()) {
        m_sensorTracks.clear();
        m_sensorTracks.resize(readings.size());
    }

    for (int i = 0; i < readings.size(); ++i) {
        const PdcSensorReading &reading = readings.at(i);
        SensorTrack &track = m_sensorTracks[i];

        if (!reading.valid || reading.distanceCm < 0.0f) {
            ++track.missingFrames;
            if (track.missingFrames > kTrackHoldFrames) {
                track.filteredDistanceCm = kInvalidFilteredDistance;
                track.recentSamplesCm.clear();
                track.confidence = 0;
            } else if (track.confidence > 0) {
                --track.confidence;
            }
            continue;
        }

        track.missingFrames = 0;

        const float previous = track.filteredDistanceCm;
        const bool hasTrack = (previous >= 0.0f);
        const bool immediateThreat = (reading.distanceCm <= kImmediateThreatCm);
        const bool suddenCloser = hasTrack && (reading.distanceCm < previous - kSuddenCloserDeltaCm);
        track.recentSamplesCm.push_back(reading.distanceCm);
        while (track.recentSamplesCm.size() > kTrackSampleWindow) {
            track.recentSamplesCm.removeFirst();
        }

        float sampleDistance = (immediateThreat || suddenCloser)
            ? reading.distanceCm
            : medianDistance(track.recentSamplesCm);
        if (hasTrack && sampleDistance > previous + kMaxReleaseStepCm) {
            sampleDistance = previous + kMaxReleaseStepCm;
        }

        if (!hasTrack || immediateThreat || suddenCloser) {
            track.filteredDistanceCm = sampleDistance;
        } else {
            const float alpha = (sampleDistance < previous) ? kApproachAlpha : kReleaseAlpha;
            track.filteredDistanceCm = previous + alpha * (sampleDistance - previous);
        }

        const int confidenceGain = (immediateThreat || suddenCloser) ? 2 : 1;
        track.confidence = qMin(kMaxConfidence, track.confidence + confidenceGain);
    }
}

PdcController::RiskCandidate PdcController::nearestConfirmedCandidate() const
{
    RiskCandidate nearest;

    for (int i = 0; i < m_sensorTracks.size(); ++i) {
        const SensorTrack &track = m_sensorTracks.at(i);
        if (track.filteredDistanceCm < 0.0f) {
            continue;
        }

        const bool confirmed = (track.confidence >= kConfirmConfidence);
        const bool immediateThreat = (track.filteredDistanceCm <= kImmediateThreatCm);
        if ((confirmed || immediateThreat) &&
            (nearest.sensorIndex < 0 || track.filteredDistanceCm < nearest.distanceCm)) {
            nearest.distanceCm = track.filteredDistanceCm;
            nearest.sensorIndex = i;
            nearest.confidence = track.confidence;
        }
    }

    return nearest;
}

float PdcController::updateDisplayDistance(float nearestCm)
{
    if (m_displayDistanceCm < 0.0f) {
        m_displayDistanceCm = nearestCm;
        return m_displayDistanceCm;
    }

    const float delta = nearestCm - m_displayDistanceCm;
    if (qAbs(delta) <= kDisplayDeadbandCm) {
        return m_displayDistanceCm;
    }

    const float alpha = (delta < 0.0f) ? kDisplayApproachAlpha : kDisplayReleaseAlpha;
    m_displayDistanceCm += alpha * delta;
    return m_displayDistanceCm;
}

float PdcController::quantizeDistance(float distanceCm) const
{
    if (distanceCm < 0.0f) {
        return -1.0f;
    }

    return qMax(0.0f, qRound(distanceCm / kDisplayStepCm) * kDisplayStepCm);
}

void PdcController::updateStateMetadata(const RiskCandidate &candidate, bool distanceHeld)
{
    m_state.distanceHeld = distanceHeld;

    if (distanceHeld) {
        m_state.confidencePercent = qMax(20, m_state.confidencePercent / 2);
        return;
    }

    if (candidate.sensorIndex < 0) {
        m_state.trackedSensorIndex = -1;
        m_state.trackedSensorName.clear();
        m_state.confidencePercent = 0;
        return;
    }

    m_state.trackedSensorIndex = candidate.sensorIndex;
    if (candidate.sensorIndex < m_state.rearSensors.size()) {
        m_state.trackedSensorName = m_state.rearSensors.at(candidate.sensorIndex).name;
    } else {
        m_state.trackedSensorName.clear();
    }

    m_state.confidencePercent =
        qBound(0, qRound((candidate.confidence * 100.0f) / kMaxConfidence), 100);
}

void PdcController::resetDistanceFilter()
{
    m_sensorTracks.clear();
    m_missingSampleFrames = 0;
    m_displayDistanceCm = kInvalidFilteredDistance;
}

void PdcController::publishState()
{
    emit stateChanged(m_state);
}
