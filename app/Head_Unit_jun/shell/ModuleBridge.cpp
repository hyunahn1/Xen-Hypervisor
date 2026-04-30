/**
 * @file ModuleBridge.cpp
 */

#include "ModuleBridge.h"
#include <QDataStream>
#include <QDebug>

ModuleBridge::ModuleBridge(const QString &socketPath, QObject *parent)
    : QObject(parent)
    , m_socketPath(socketPath)
    , m_server(new QLocalServer(this))
{
}

ModuleBridge::~ModuleBridge()
{
    QLocalServer::removeServer(m_socketPath);
}

bool ModuleBridge::listen()
{
    QLocalServer::removeServer(m_socketPath); // 이전 소켓 파일 정리
    if (!m_server->listen(m_socketPath)) {
        qCritical() << "[ModuleBridge] failed to listen on" << m_socketPath
                    << ":" << m_server->errorString();
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, &ModuleBridge::onNewConnection);
    qDebug() << "[ModuleBridge] listening on" << m_socketPath;
    return true;
}

void ModuleBridge::onNewConnection()
{
    if (m_socket) {
        // 기존 연결 교체 (모듈 재시작 시)
        m_socket->disconnectFromServer();
        m_socket->deleteLater();
    }
    m_socket = m_server->nextPendingConnection();
    connect(m_socket, &QLocalSocket::readyRead,    this, &ModuleBridge::onReadyRead);
    connect(m_socket, &QLocalSocket::disconnected, this, &ModuleBridge::onSocketDisconnected);
    qDebug() << "[ModuleBridge] module connected on" << m_socketPath;
}

void ModuleBridge::onSocketDisconnected()
{
    qDebug() << "[ModuleBridge] module disconnected on" << m_socketPath;
    m_socket->deleteLater();
    m_socket = nullptr;
}

void ModuleBridge::onReadyRead()
{
    m_readBuf.append(m_socket->readAll());
    processBuffer();
}

void ModuleBridge::processBuffer()
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

void ModuleBridge::dispatchFrame(HuProtocol::MsgType type, const QByteArray &payload)
{
    using MT = HuProtocol::MsgType;
    QDataStream ds(payload);
    ds.setByteOrder(QDataStream::BigEndian);

    switch (type) {
    case MT::ModuleReady: {
        quint64 winId;
        ds >> winId;
        emit moduleReady(winId);
        break;
    }
    case MT::GearChangeRequest: {
        quint8 g, src;
        ds >> g >> src;
        const QString source = (src == 1) ? "button" : "touch";
        emit gearChangeRequested(static_cast<GearState>(g), source);
        break;
    }
    case MT::AmbientColorSet: {
        quint8 r, gr, b, br;
        ds >> r >> gr >> b >> br;
        emit ambientColorChanged(r, gr, b, br);
        break;
    }
    case MT::AmbientOff:
        emit ambientOff();
        break;
    case MT::SettingsChanged: {
        QVariantMap changes;
        ds >> changes;
        emit settingsChanged(changes);
        break;
    }
    default:
        qWarning() << "[ModuleBridge] unknown msg type" << static_cast<quint32>(type);
        break;
    }
}

// ── 송신 ─────────────────────────────────────────────────────────────

void ModuleBridge::sendFrame(HuProtocol::MsgType type, const QByteArray &payload)
{
    if (!m_socket) return;
    m_socket->write(HuProtocol::encodeFrame(type, payload));
}

void ModuleBridge::sendShow(const QRect &geo)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << qint32(geo.x()) << qint32(geo.y()) << qint32(geo.width()) << qint32(geo.height());
    sendFrame(HuProtocol::MsgType::ShowModule, p);
}

void ModuleBridge::sendHide()
{
    sendFrame(HuProtocol::MsgType::HideModule);
}

void ModuleBridge::sendGearState(GearState gear)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint8>(gear);
    sendFrame(HuProtocol::MsgType::GearStateUpdate, p);
}

void ModuleBridge::sendVehicleSpeed(float kmh)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << kmh;
    sendFrame(HuProtocol::MsgType::VehicleSpeedUpdate, p);
}

void ModuleBridge::sendBattery(float voltage, float percent)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << voltage << percent;
    sendFrame(HuProtocol::MsgType::BatteryUpdate, p);
}

void ModuleBridge::sendIpcStatus(bool connected)
{
    QByteArray p;
    QDataStream ds(&p, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint8>(connected ? 1 : 0);
    sendFrame(HuProtocol::MsgType::IpcStatusUpdate, p);
}

void ModuleBridge::sendShutdown()
{
    sendFrame(HuProtocol::MsgType::ShellShutdown);
}
