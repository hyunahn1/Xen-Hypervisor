/**
 * @file ShellClient.h
 * @brief 각 모듈 프로세스가 사용하는 Shell IPC 클라이언트
 *
 * 사용법:
 *   ShellClient *client = new ShellClient("/tmp/hu_shell_media.sock");
 *   connect(client, &ShellClient::showRequested, window, [](QRect geo){ window->setGeometry(geo); window->show(); });
 *   client->connectToShell();
 */

#ifndef SHELLCLIENT_H
#define SHELLCLIENT_H

#include "ShellProtocol.h"
#include "IVehicleDataProvider.h"  // GearState enum

#include <QLocalSocket>
#include <QRect>
#include <QVariantMap>
#include <QTimer>

class ShellClient : public QObject
{
    Q_OBJECT

public:
    explicit ShellClient(const QString &socketPath, QObject *parent = nullptr);

    /** Shell 소켓에 연결 시도 (비동기, 실패 시 재시도) */
    void connectToShell();

    bool isConnected() const;

    // ── Module → Shell 전송 메서드 ─────────────────────────
    void notifyReady(quint64 winId);
    void requestGearChange(GearState gear, const QString &source);
    void sendAmbientColor(quint8 r, quint8 g, quint8 b, quint8 brightness);
    void sendAmbientOff();
    void sendSettingsChanged(const QVariantMap &changes);

signals:
    void connected();
    void disconnected();

    // ── Shell → Module 수신 시그널 ─────────────────────────
    void showRequested(QRect geometry);
    void hideRequested();
    void gearStateUpdated(GearState gear);
    void vehicleSpeedUpdated(float kmh);
    void batteryUpdated(float voltage, float percent);
    void ipcStatusUpdated(bool connected);
    void shellShutdown();

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void tryReconnect();

private:
    void sendFrame(HuProtocol::MsgType type, const QByteArray &payload = {});
    void processBuffer();
    void dispatchFrame(HuProtocol::MsgType type, const QByteArray &payload);

    QString        m_socketPath;
    QLocalSocket  *m_socket;
    QByteArray     m_readBuf;
    QTimer        *m_reconnectTimer;
};

#endif // SHELLCLIENT_H
