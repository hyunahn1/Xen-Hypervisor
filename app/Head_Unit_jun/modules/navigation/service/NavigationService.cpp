#include "NavigationService.h"

NavigationService::NavigationService(QObject *parent)
    : QObject(parent)
    , m_url("https://www.openstreetmap.org/#map=13/37.5665/126.9780")
{}

void NavigationService::setDestination(double lat, double lon, int zoom)
{
    m_url = QUrl(QString("https://www.openstreetmap.org/#map=%1/%2/%3")
                 .arg(zoom).arg(lat).arg(lon));
    emit mapUrlChanged(m_url);
}
