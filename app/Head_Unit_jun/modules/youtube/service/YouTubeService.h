/**
 * @file YouTubeService.h
 * @brief YouTube 모듈 서비스 레이어 — URL 및 재생 상태 관리
 */
#ifndef YOUTUBESERVICE_H
#define YOUTUBESERVICE_H

#include <QObject>
#include <QUrl>

class YouTubeService : public QObject
{
    Q_OBJECT
public:
    explicit YouTubeService(QObject *parent = nullptr);
    QUrl currentUrl() const { return m_url; }
    void setUrl(const QUrl &url);

signals:
    void urlChanged(const QUrl &url);

private:
    QUrl m_url { "https://www.youtube.com" };
};
#endif // YOUTUBESERVICE_H
