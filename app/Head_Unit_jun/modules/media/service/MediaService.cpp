/**
 * @file MediaService.cpp
 */

#include "MediaService.h"
#include "MediaPlayer.h"
#include "PlaylistModel.h"

MediaService::MediaService(QObject *parent)
    : QObject(parent)
    , m_player(new MediaPlayer(this))
    , m_playlist(new PlaylistModel(this))
{
    connect(m_player, &MediaPlayer::stateChanged,
            this, &MediaService::playbackStateChanged);
    connect(m_player, &MediaPlayer::positionChanged,
            this, &MediaService::positionChanged);
    connect(m_player, &MediaPlayer::durationChanged,
            this, &MediaService::durationChanged);
    connect(m_player, &MediaPlayer::coverArtChanged,
            this, &MediaService::coverArtChanged);
    connect(m_player, &MediaPlayer::errorOccurred,
            this, &MediaService::errorOccurred);
    connect(m_player, &MediaPlayer::trackFinished,
            this, &MediaService::onTrackFinished);
}

void MediaService::playTrack(int row)
{
    if (row < 0 || row >= m_playlist->rowCount()) return;
    m_currentTrack = row;
    const QUrl url = m_playlist->urlAt(row);
    m_player->setSource(url, true);
    emit trackChanged(m_playlist->titleAt(row), QString());
}

void MediaService::play()         { m_player->play(); }
void MediaService::pause()        { m_player->pause(); }
void MediaService::togglePlayPause()
{
    if (m_player->isPlaying()) m_player->pause();
    else                       m_player->play();
}

void MediaService::next()
{
    if (m_playlist->rowCount() == 0) return;
    playTrack((m_currentTrack + 1) % m_playlist->rowCount());
}

void MediaService::prev()
{
    if (m_playlist->rowCount() == 0) return;
    int idx = m_currentTrack - 1;
    if (idx < 0) idx = m_playlist->rowCount() - 1;
    playTrack(idx);
}

void MediaService::setVolume(int percent) { m_player->setVolume(percent); }
void MediaService::seekTo(qint64 ms)      { m_player->seekTo(ms); }

void MediaService::loadFolder(const QString &path)
{
    m_playlist->loadFromDirectory(path);
    if (m_playlist->rowCount() > 0) playTrack(0);
}

void MediaService::onTrackFinished() { next(); }
