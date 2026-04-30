/**
 * @file MediaPlayer.h
 * @brief QMediaPlayer wrapper - real audio when HU_MULTIMEDIA_AVAILABLE, stub otherwise
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QUrl>
#include <QPixmap>
#include <QtCore/qglobal.h>

#ifdef HU_MULTIMEDIA_AVAILABLE
class QMediaPlayer;
#if QT_VERSION_MAJOR >= 6
class QAudioOutput;
#endif
#endif // HU_MULTIMEDIA_AVAILABLE

class MediaPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    ~MediaPlayer() override = default;

    void play();
    void pause();
    void stop();
    void setSource(const QUrl &url, bool autoPlay = false);
    void setVolume(int percent);   // 0–100
    void seekTo(qint64 ms);

    bool    isPlaying() const;
    qint64  position()  const;
    qint64  duration()  const;
    int     volume()    const;

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void stateChanged(bool playing);
    void trackFinished();
    void errorOccurred(const QString &msg);
    // Emitted when cover art is loaded; null pixmap = no art embedded
    void coverArtChanged(const QPixmap &art);

private:
#ifdef HU_MULTIMEDIA_AVAILABLE
    QMediaPlayer *m_qPlayer = nullptr;
#if QT_VERSION_MAJOR >= 6
    QAudioOutput *m_audioOut = nullptr;
#endif
#endif // HU_MULTIMEDIA_AVAILABLE
    bool  m_playing    = false;
    bool  m_playOnLoad = false;
    int   m_volume     = 50;
};

#endif // MEDIAPLAYER_H
