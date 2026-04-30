/**
 * @file PlaylistModel.cpp
 * @brief Playlist model - loads audio files from directory
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "PlaylistModel.h"
#include <QDir>
#include <QFileInfo>

static const QStringList kAudioFilters = {
    "*.mp3", "*.wav", "*.flac", "*.m4a", "*.ogg", "*.aac"
};

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int PlaylistModel::rowCount(const QModelIndex &) const
{
    return m_urls.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_urls.size()) return {};

    switch (role) {
    case UrlRole:
    case Qt::UserRole:    return m_urls[index.row()];
    case TitleRole:
    case Qt::DisplayRole: return m_titles.value(index.row(), "Unknown");
    case ArtistRole:      return m_artists.value(index.row(), "Unknown");
    }
    return {};
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return { {UrlRole, "url"}, {TitleRole, "title"}, {ArtistRole, "artist"} };
}

void PlaylistModel::loadFromDirectory(const QString &dirPath)
{
    beginResetModel();
    m_urls.clear();
    m_titles.clear();
    m_artists.clear();

    QDir dir(dirPath);
    const auto files = dir.entryInfoList(kAudioFilters, QDir::Files, QDir::Name);
    for (const QFileInfo &fi : files) {
        m_urls.append(QUrl::fromLocalFile(fi.absoluteFilePath()));
        m_titles.append(fi.completeBaseName());
        m_artists.append("Unknown Artist");
    }
    endResetModel();
}

void PlaylistModel::addFile(const QUrl &url)
{
    const int row = m_urls.size();
    beginInsertRows({}, row, row);
    m_urls.append(url);
    QFileInfo fi(url.toLocalFile());
    m_titles.append(fi.completeBaseName());
    m_artists.append("Unknown Artist");
    endInsertRows();
}

void PlaylistModel::clear()
{
    beginResetModel();
    m_urls.clear();
    m_titles.clear();
    m_artists.clear();
    endResetModel();
}

QUrl PlaylistModel::urlAt(int row) const
{
    return (row >= 0 && row < m_urls.size()) ? m_urls[row] : QUrl();
}

QString PlaylistModel::titleAt(int row) const
{
    return m_titles.value(row, "Unknown");
}
