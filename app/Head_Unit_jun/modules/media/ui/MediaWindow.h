/**
 * @file MediaWindow.h
 * @brief Media 모듈 UI — Now Playing, 플레이리스트, 재생 컨트롤
 */

#ifndef MEDIAWINDOW_H
#define MEDIAWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QListView>
#include <QPixmap>
#include <QColor>

class MediaPlayer;
class PlaylistModel;
class GearStateManager;
class GearPanel;

class MediaWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MediaWindow(GearStateManager *gearState, QWidget *parent = nullptr);

signals:
    void accentColorChanged(const QColor &color);

private slots:
    void onPlayPause();
    void onPrev();
    void onNext();
    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onStateChanged(bool playing);
    void onTrackFinished();
    void onPlaylistItemActivated(const QModelIndex &index);
    void onAddFolder();
    void onProgressSliderMoved(int value);
    void onVolumeChanged(int value);
    void onCoverArtChanged(const QPixmap &art);
    void onError(const QString &msg);

private:
    void    playTrack(int row);
    void    showPlaceholderArt();
    QString formatTime(qint64 ms) const;
    void    applyAccentColor(const QColor &color);
    static  QColor extractVibrantColor(const QPixmap &pixmap);

    MediaPlayer   *m_player;
    PlaylistModel *m_playlist;
    GearPanel     *m_gearPanel;

    QLabel      *m_albumArt       = nullptr;
    QLabel      *m_trackTitle     = nullptr;
    QLabel      *m_artistName     = nullptr;
    QPushButton *m_prevBtn        = nullptr;
    QPushButton *m_playPauseBtn   = nullptr;
    QPushButton *m_nextBtn        = nullptr;
    QSlider     *m_progressSlider = nullptr;
    QLabel      *m_posLabel       = nullptr;
    QLabel      *m_durLabel       = nullptr;
    QSlider     *m_volumeSlider   = nullptr;
    QListView   *m_playlistView   = nullptr;

    int    m_currentTrack = -1;
    bool   m_seeking      = false;
    QColor m_accentColor  { 0, 212, 170 };
};

#endif // MEDIAWINDOW_H
