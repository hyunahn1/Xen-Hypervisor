/**
 * @file MediaPlayer.cpp
 * @brief QMediaPlayer wrapper - cover art, error handling, graceful fallback
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "MediaPlayer.h"

#include <QTimer>

#ifdef HU_MULTIMEDIA_AVAILABLE
#include <QMediaPlayer>
#include <QImage>
#if QT_VERSION_MAJOR >= 6
#include <QAudioOutput>
#include <QMediaMetaData>
#endif
#endif // HU_MULTIMEDIA_AVAILABLE

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    m_qPlayer = new QMediaPlayer(this);
#if QT_VERSION_MAJOR >= 6
    m_audioOut = new QAudioOutput(this);
    m_qPlayer->setAudioOutput(m_audioOut);
    m_audioOut->setVolume(0.5f);
#else
    m_qPlayer->setVolume(50);
#endif

    connect(m_qPlayer, &QMediaPlayer::positionChanged,
            this, [this](qint64 pos) {
        // Consider autoplay "successful" only when media clock advances.
        if (pos > 0 && m_playOnLoad)
            m_playOnLoad = false;
        emit positionChanged(pos);
    });

    // durationChanged: forward to UI *and* trigger deferred play.
    // duration > 0 is the most reliable "media is ready" indicator on macOS/AVFoundation.
#if QT_VERSION_MAJOR >= 6
    connect(m_qPlayer, &QMediaPlayer::durationChanged,
            this, [this](qint64 dur) {
        emit durationChanged(dur);
        // Retry play while autoplay is pending.
        if (dur > 0 && m_playOnLoad)
            m_qPlayer->play();
    });
#else
    connect(m_qPlayer, &QMediaPlayer::durationChanged,
            this, &MediaPlayer::durationChanged);
#endif

#if QT_VERSION_MAJOR >= 6
    connect(m_qPlayer, &QMediaPlayer::playbackStateChanged,
            this, [this](QMediaPlayer::PlaybackState state) {
        m_playing = (state == QMediaPlayer::PlayingState);
        emit stateChanged(m_playing);
    });
    connect(m_qPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            emit trackFinished();
        if (status == QMediaPlayer::InvalidMedia)
            emit errorOccurred("Cannot play: unsupported or corrupt file");
        // Safety net: retry play on load/buffer ready.
        // Do NOT reset m_playOnLoad here; position advance resets it.
        if ((status == QMediaPlayer::LoadedMedia ||
             status == QMediaPlayer::BufferedMedia) && m_playOnLoad)
            m_qPlayer->play();
    });
    connect(m_qPlayer, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error, const QString &msg) {
        emit errorOccurred(msg);
    });
    // Cover art: read from embedded metadata when it becomes available
    connect(m_qPlayer, &QMediaPlayer::metaDataChanged, this, [this]() {
        const QVariant v =
            m_qPlayer->metaData().value(QMediaMetaData::CoverArtImage);
        if (v.isValid())
            emit coverArtChanged(QPixmap::fromImage(v.value<QImage>()));
        else
            emit coverArtChanged(QPixmap()); // no embedded art
    });
#else
    connect(m_qPlayer, &QMediaPlayer::stateChanged,
            this, [this](QMediaPlayer::State state) {
        m_playing = (state == QMediaPlayer::PlayingState);
        emit stateChanged(m_playing);
    });
    connect(m_qPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            emit trackFinished();
        if (status == QMediaPlayer::InvalidMedia)
            emit errorOccurred("Cannot play: unsupported or corrupt file");
    });
    connect(m_qPlayer,
            QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
            this, [this](QMediaPlayer::Error) {
        emit errorOccurred(m_qPlayer->errorString());
    });
#endif // QT_VERSION_MAJOR
#endif // HU_MULTIMEDIA_AVAILABLE
}

void MediaPlayer::play()
{
#ifdef HU_MULTIMEDIA_AVAILABLE
#if QT_VERSION_MAJOR >= 6
    m_playOnLoad = true;
    // Attempt immediately; AVFoundation may ignore this if media is still loading.
    // durationChanged(>0) and mediaStatusChanged(Loaded/Buffered) will retry.
    m_qPlayer->play();
    // Extra safety: retry several times for backend timing races.
    // If playback remains stuck at 0 ms, nudge position by 1 ms.
    for (const int delayMs : {120, 300, 600, 1000}) {
        QTimer::singleShot(delayMs, this, [this, delayMs] {
            if (!m_playOnLoad) return;
            const auto state = m_qPlayer->playbackState();
            const qint64 pos = m_qPlayer->position();
            if (pos > 0) {
                m_playOnLoad = false;
                return;
            }
            if (delayMs >= 300) {
                // AVFoundation can report PlayingState while staying at 0ms.
                // Force a tiny seek + pause/play to kick decoder clock.
                m_qPlayer->setPosition(1);
                if (state == QMediaPlayer::PlayingState)
                    m_qPlayer->pause();
            }
            m_qPlayer->play();
        });
    }
#else
    m_qPlayer->play();
#endif
#else
    m_playing = true;
    emit stateChanged(true);
#endif
}

void MediaPlayer::pause()
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    m_playOnLoad = false;
    m_qPlayer->pause();
#else
    m_playing = false;
    emit stateChanged(false);
#endif
}

void MediaPlayer::stop()
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    m_playOnLoad = false;
    m_qPlayer->stop();
#else
    m_playing = false;
    emit stateChanged(false);
#endif
}

void MediaPlayer::setSource(const QUrl &url, bool autoPlay)
{
    m_playOnLoad = autoPlay;
    emit coverArtChanged(QPixmap());

#ifdef HU_MULTIMEDIA_AVAILABLE
#if QT_VERSION_MAJOR >= 6
    m_qPlayer->setSource(url);
    if (autoPlay) {
        // Some backends may load metadata synchronously while setting source.
        // Start immediately and keep retry logic alive via m_playOnLoad.
        m_qPlayer->play();
        for (const int delayMs : {120, 300, 600, 1000}) {
            QTimer::singleShot(delayMs, this, [this, delayMs] {
                if (!m_playOnLoad) return;
                const auto state = m_qPlayer->playbackState();
                const qint64 pos = m_qPlayer->position();
                if (pos > 0) {
                    m_playOnLoad = false;
                    return;
                }
                if (delayMs >= 300) {
                    m_qPlayer->setPosition(1);
                    if (state == QMediaPlayer::PlayingState)
                        m_qPlayer->pause();
                }
                m_qPlayer->play();
            });
        }
    }
#else
    m_qPlayer->setMedia(QMediaContent(url));
    if (autoPlay)
        m_qPlayer->play();
#endif
#else
    Q_UNUSED(url)
    Q_UNUSED(autoPlay)
#endif
}

void MediaPlayer::setVolume(int percent)
{
    m_volume = qBound(0, percent, 100);
#ifdef HU_MULTIMEDIA_AVAILABLE
#if QT_VERSION_MAJOR >= 6
    m_audioOut->setVolume(m_volume / 100.0f);
#else
    m_qPlayer->setVolume(m_volume);
#endif
#endif
}

void MediaPlayer::seekTo(qint64 ms)
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    m_qPlayer->setPosition(ms);
#else
    Q_UNUSED(ms)
#endif
}

bool MediaPlayer::isPlaying() const
{
#ifdef HU_MULTIMEDIA_AVAILABLE
#if QT_VERSION_MAJOR >= 6
    if (m_qPlayer->playbackState() != QMediaPlayer::PlayingState)
        return false;
    // Treat "Playing but frozen at 0ms" as not playing for UI toggle logic.
    if (m_playOnLoad && m_qPlayer->position() == 0)
        return false;
    return true;
#else
    return m_qPlayer->state() == QMediaPlayer::PlayingState;
#endif
#else
    return m_playing;
#endif
}

qint64 MediaPlayer::position() const
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    return m_qPlayer->position();
#else
    return 0;
#endif
}

qint64 MediaPlayer::duration() const
{
#ifdef HU_MULTIMEDIA_AVAILABLE
    return m_qPlayer->duration();
#else
    return 0;
#endif
}

int MediaPlayer::volume() const { return m_volume; }
