#include "SocketCanPdcProvider.h"

#include <QDateTime>
#include <QDebug>
#include <QByteArray>

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

namespace {
constexpr canid_t kObservedCanId = 0x123;
constexpr canid_t kFourSensorCanId = 0x350;
constexpr float kMinDistanceCm = 2.0f;
constexpr float kMaxDistanceCm = 400.0f;

const QString kCenterSensorName = QStringLiteral("rear_center");

const QStringList kFourSensorNames = {
    QStringLiteral("rear_left"),
    QStringLiteral("rear_mid_left"),
    QStringLiteral("rear_mid_right"),
    QStringLiteral("rear_right")
};

bool distanceValid(float distanceCm)
{
    return distanceCm >= kMinDistanceCm && distanceCm <= kMaxDistanceCm;
}

quint16 le16(const __u8 *data)
{
    return static_cast<quint16>(data[0]) |
           static_cast<quint16>(data[1] << 8);
}
}

SocketCanPdcProvider::SocketCanPdcProvider(const QString &interfaceName, QObject *parent)
    : IPdcSensorProvider(parent)
    , m_interfaceName(interfaceName)
{
}

SocketCanPdcProvider::~SocketCanPdcProvider()
{
    stop();
}

void SocketCanPdcProvider::start()
{
    if (m_socket >= 0 || !openSocket()) {
        return;
    }

    m_notifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &SocketCanPdcProvider::onCanReadyRead);
}

void SocketCanPdcProvider::stop()
{
    closeSocket();
}

bool SocketCanPdcProvider::openSocket()
{
    m_socket = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_socket < 0) {
        emit faultChanged(QStringLiteral("Failed to create CAN socket"));
        return false;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    const QByteArray ifName = m_interfaceName.toLatin1();
    std::strncpy(ifr.ifr_name, ifName.constData(), IFNAMSIZ - 1);
    if (::ioctl(m_socket, SIOCGIFINDEX, &ifr) < 0) {
        emit faultChanged(QStringLiteral("Failed to resolve %1").arg(m_interfaceName));
        closeSocket();
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(m_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        emit faultChanged(QStringLiteral("Failed to bind CAN socket to %1").arg(m_interfaceName));
        closeSocket();
        return false;
    }

    qInfo() << "[PDC] SocketCAN provider listening on" << m_interfaceName;
    return true;
}

void SocketCanPdcProvider::closeSocket()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }

    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
}

void SocketCanPdcProvider::onCanReadyRead()
{
    if (m_socket < 0) {
        return;
    }

    struct can_frame frame;
    const ssize_t n = ::read(m_socket, &frame, sizeof(frame));
    if (n < static_cast<ssize_t>(sizeof(frame))) {
        return;
    }

    const canid_t canId = frame.can_id & CAN_EFF_MASK;
    if (canId == kFourSensorCanId && frame.can_dlc >= 8) {
        emitFourRearDistances(le16(&frame.data[0]),
                              le16(&frame.data[2]),
                              le16(&frame.data[4]),
                              le16(&frame.data[6]));
        return;
    }

    if (canId == kObservedCanId && frame.can_dlc >= 2) {
        // Live Pi capture currently shows 0x123 as: 00 48/49 00 ...
        // The car has one rear HC-SR04-style module, so expose it as rear_center.
        emitUniformRearDistance(static_cast<float>(frame.data[1]));
    }
}

void SocketCanPdcProvider::emitUniformRearDistance(float distanceCm)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QVector<PdcSensorReading> readings;
    readings.reserve(1);

    PdcSensorReading reading;
    reading.name = kCenterSensorName;
    reading.distanceCm = distanceCm;
    reading.valid = distanceValid(distanceCm);
    reading.timestampMs = now;
    readings.push_back(reading);

    emit readingsChanged(readings);
}

void SocketCanPdcProvider::emitFourRearDistances(float rl, float rml, float rmr, float rr)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const float values[] = {rl, rml, rmr, rr};
    QVector<PdcSensorReading> readings;
    readings.reserve(kFourSensorNames.size());

    for (int i = 0; i < kFourSensorNames.size(); ++i) {
        PdcSensorReading reading;
        reading.name = kFourSensorNames.at(i);
        reading.distanceCm = values[i];
        reading.valid = distanceValid(values[i]);
        reading.timestampMs = now;
        readings.push_back(reading);
    }

    emit readingsChanged(readings);
}
