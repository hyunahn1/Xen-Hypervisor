#ifndef PDCCONTROLLER_H
#define PDCCONTROLLER_H

#include "PdcTypes.h"

#include <QObject>
#include <QTimer>
#include <QVector>

class IPdcSensorProvider;

class PdcController : public QObject
{
    Q_OBJECT

public:
    explicit PdcController(IPdcSensorProvider *provider, QObject *parent = nullptr);

    const PdcState &state() const { return m_state; }
    void setActive(bool active);
    void setVehicleSpeed(float kmh);

signals:
    void stateChanged(const PdcState &state);

private slots:
    void onReadingsChanged(const QVector<PdcSensorReading> &readings);
    void onProviderFault(const QString &message);
    void markStale();

private:
    struct SensorTrack {
        float filteredDistanceCm = -1.0f;
        QVector<float> recentSamplesCm;
        int confidence = 0;
        int missingFrames = 0;
    };

    struct RiskCandidate {
        float distanceCm = kInvalidFilteredDistance;
        int sensorIndex = -1;
        int confidence = 0;
    };

    PdcWarningLevel levelForDistance(float distanceCm, PdcWarningLevel previousLevel) const;
    void updateSensorTracks(const QVector<PdcSensorReading> &readings);
    RiskCandidate nearestConfirmedCandidate() const;
    float updateDisplayDistance(float nearestCm);
    float quantizeDistance(float distanceCm) const;
    void updateStateMetadata(const RiskCandidate &candidate, bool distanceHeld);
    void resetDistanceFilter();
    void publishState();

    IPdcSensorProvider *m_provider = nullptr;
    PdcState m_state;
    QTimer m_staleTimer;
    QVector<SensorTrack> m_sensorTracks;
    int m_missingSampleFrames = 0;
    float m_displayDistanceCm = kInvalidFilteredDistance;
    float m_vehicleSpeedKmh = 0.0f;

    static constexpr float kMaxActiveSpeedKmh = 10.0f;
    static constexpr int kStaleTimeoutMs = 450;
    static constexpr int kMaxMissingSampleFrames = 3;
    static constexpr int kTrackHoldFrames = 3;
    static constexpr int kTrackSampleWindow = 3;
    static constexpr int kMaxConfidence = 4;
    static constexpr int kConfirmConfidence = 2;
    static constexpr float kInvalidFilteredDistance = -1.0f;
    static constexpr float kApproachAlpha = 0.85f;
    static constexpr float kReleaseAlpha = 0.45f;
    static constexpr float kImmediateThreatCm = 35.0f;
    static constexpr float kSuddenCloserDeltaCm = 18.0f;
    static constexpr float kMaxReleaseStepCm = 12.0f;
    static constexpr float kDisplayApproachAlpha = 0.85f;
    static constexpr float kDisplayReleaseAlpha = 0.45f;
    static constexpr float kDisplayDeadbandCm = 0.5f;
    static constexpr float kDisplayStepCm = 1.0f;
    static constexpr float kWarningHysteresisCm = 5.0f;
};

#endif // PDCCONTROLLER_H
