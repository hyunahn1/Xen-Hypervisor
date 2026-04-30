#include "YouTubeService.h"

YouTubeService::YouTubeService(QObject *parent) : QObject(parent) {}

void YouTubeService::setUrl(const QUrl &url)
{
    if (m_url == url) return;
    m_url = url;
    emit urlChanged(m_url);
}
