/**
 * @file ShellClient.cpp
 */

#include "ShellClient.h"
#include <QDataStream>
#include <QDebug>

ShellClient::ShellClient(const QString &socketPath, QObject *parent)
    : QObject(parent)
    , m_socketPath(socketPath)
    , m_socket(new QLocalSocket(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setInterval(1000);
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QLocalSocket::connected,    this, &ShellClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &ShellClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead,    this, &ShellClient::onReadyRead);
    connect(m_reconnectTimer, &QTimer::timeout,    this, &ShellClient::tryReconnect);
}

void ShellClient::connectToShell()
{
    m_socket->connectToServer(m_socketPath);
}

bool ShellClient::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

// ── Shell 연결 이벤트 ─────────────────────────────────────────────────

void ShellClient::onConnected()
{
    qDebug() << "[ShellClient] connected to" << m_socketPath;
    emit connected();
}

void ShellClient::onDisconnected()
{
    qDebug() << "[ShellClient] disconnected; retrying in 1s";
    emit disconnected();
    m_reconnectTimer->start();
}

void ShellClient::tryReconnect()
{
    if (m_socket->state() == QLocalSocket::UnconnectedState)
        m_socket->connectToServer(m_socketPath);
}

// ── 수신 처리 ─────────────────────────────────────────────────────────

void ShellClient::onReadyRead()
{
    m_readBuf.append(m_socket->readAll());
    processBuffer();
}

void ShellClient::processBuffer()
{
    while (true) {
        HuProtocol::MsgType type;
        QByteArray payload;
        int consumed = 0;
        if (!HuProtocol::decodeFrame(m_readBuf, type, payload, consumed))
            break;
        m_readBuf.remove(0, consumed);
        dispatchFrame(type, payload);
    }
}

void ShellClient::dispatchFrame(HuProtocol::MsgType type, const QByteArray &payload)
{
    using MT = HuProtocol::MsgType;
    QDataStream ds(payload);
    ds.setByteOrder(QDataStream::BigEndian);

    switch (type) {
    case MT::ShowModule: {
        qint32 x, y, w, h;
        ds >> x >> y >> w >> h;
        emit showRequested(QRect(x, y, w, h));
        break;
    }
    case MT::HideModule:
        emit hideRequested();
        break;
    case MT::GearStateUpdate: {
        quint8 g;
        ds >> g;
        emit gearStateUpdated(static_cast<GearState>(g));
        break;
    }
    case MT::VehicleSpeedUpdate: {
        float kmh;
        ds >> kmh;
        emit vehicleSpeedUpdated(kmh);
        break;
    }
    case MT::BatteryUpdate: {
        float v, pct;
        ds >> v >> pct;
        emit batteryUpdated(v, pct);
        break;
    }
    case MT::IpcStatusUpdate: {
        quint8 status;
        ds >> status;
        emit ipcStatusUpdated(status != 0);
        break;
    }
    case MT::ShellShutdown:
        emit shellShutdown();
        break;
    default:
        qWarning() << "[ShellClient] unknown msg type" << static_cast<quint32>(type);
        break;
    }
}

// ── 송신 헬퍼 ─────────────────────────────────────────────────────────

void ShellClient::sendFrame(HuProtocol::MsgType type, const QByteArray &payload)
{
    if (!isConnected()) return;
    m_socket->write(HuProtocol::encodeFrame(type, payload));
}

void ShellClient::notifyReady(quint64 winId)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << winId;
    sendFrame(HuProtocol::MsgType::ModuleReady, p);
}

void ShellClient::requestGearChange(GearState gear, const QString &source)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    quint8 src = (source == "button") ? 1 : 0;
    ds << static_cast<quint8>(gear) << src;
    sendFrame(HuProtocol::MsgType::GearChangeRequest, p);
}

void ShellClient::sendAmbientColor(quint8 r, quint8 g, quint8 b, quint8 brightness)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << r << g << b << brightness;
    sendFrame(HuProtocol::MsgType::AmbientColorSet, p);
}

void ShellClient::sendAmbientOff()
{
    sendFrame(HuProtocol::MsgType::AmbientOff);
}

void ShellClient::sendSettingsChanged(const QVariantMap &changes)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << changes;
    sendFrame(HuProtocol::MsgType::SettingsChanged, p);
}
