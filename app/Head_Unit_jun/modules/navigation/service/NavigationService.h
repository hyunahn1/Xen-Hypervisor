/**
 * @file NavigationService.h
 * @brief Navigation 모듈 서비스 레이어 — 위치/지도 URL 관리
 */
#ifndef NAVIGATIONSERVICE_H
#define NAVIGATIONSERVICE_H

#include <QObject>
#include <QUrl>

class NavigationService : public QObject
{
    Q_OBJECT
public:
    explicit NavigationService(QObject *parent = nullptr);
    QUrl mapUrl() const { return m_url; }

public slots:
    void setDestination(double lat, double lon, int zoom = 13);

signals:
    void mapUrlChanged(const QUrl &url);

private:
    QUrl m_url;
};
#endif // NAVIGATIONSERVICE_H
