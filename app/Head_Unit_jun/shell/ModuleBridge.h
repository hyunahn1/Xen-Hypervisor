/**
 * @file ModuleBridge.h
 * @brief Shell 측 IPC 서버 — 모듈 1개당 QLocalServer 1개
 */

#ifndef MODULEBRIDGE_H
#define MODULEBRIDGE_H

#include "ShellProtocol.h"
#include "IVehicleDataProvider.h"

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QRect>
#include <QVariantMap>

class ModuleBridge : public QObject
{
    Q_OBJECT

public:
    explicit ModuleBridge(const QString &socketPath, QObject *parent = nullptr);
    ~ModuleBridge();

    bool listen();

    // ── Shell → Module ───────────────────────────────
    void sendShow(const QRect &geometry);
    void sendHide();
    void sendGearState(GearState gear);
    void sendVehicleSpeed(float kmh);
    void sendBattery(float voltage, float percent);
    void sendIpcStatus(bool connected);
    void sendShutdown();

    bool isModuleConnected() const { return m_socket != nullptr; }

signals:
    // ── Module → Shell ───────────────────────────────
    void moduleReady(quint64 winId);
    void gearChangeRequested(GearState gear, const QString &source);
    void ambientColorChanged(quint8 r, quint8 g, quint8 b, quint8 brightness);
    void ambientOff();
    void settingsChanged(const QVariantMap &changes);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();

private:
    void sendFrame(HuProtocol::MsgType type, const QByteArray &payload = {});
    void processBuffer();
    void dispatchFrame(HuProtocol::MsgType type, const QByteArray &payload);

    QString        m_socketPath;
    QLocalServer  *m_server;
    QLocalSocket  *m_socket = nullptr;
    QByteArray     m_readBuf;
};

#endif // MODULEBRIDGE_H
