/**
 * @file MediaService.h
 * @brief Media 비즈니스 로직 — MediaPlayer + Playlist 관리
 *
 * UI 의존성 없음. ShellClient 시그널에 연결해서 기어/속도 데이터를 받고,
 * 재생 상태를 시그널로 내보낸다.
 */

#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include <QObject>
#include <QPixmap>

class MediaPlayer;
class PlaylistModel;

class MediaService : public QObject
{
    Q_OBJECT

public:
    explicit MediaService(QObject *parent = nullptr);

    MediaPlayer   *player()   const { return m_player; }
    PlaylistModel *playlist() const { return m_playlist; }

    void playTrack(int row);

public slots:
    void play();
    void pause();
    void togglePlayPause();
    void next();
    void prev();
    void setVolume(int percent);
    void seekTo(qint64 ms);
    void loadFolder(const QString &path);

signals:
    void playbackStateChanged(bool playing);
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void trackChanged(const QString &title, const QString &artist);
    void coverArtChanged(const QPixmap &art);
    void errorOccurred(const QString &msg);

private slots:
    void onTrackFinished();

private:
    MediaPlayer   *m_player;
    PlaylistModel *m_playlist;
    int            m_currentTrack = -1;
};

#endif // MEDIASERVICE_H
