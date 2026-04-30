/**
 * @file ShellProtocol.h
 * @brief Shell ↔ Module IPC 메시지 타입 및 직렬화 유틸리티
 *
 * 프레임 포맷 (QDataStream 빅엔디안):
 *   [uint32: payload_length] [uint32: MsgType] [payload bytes...]
 */

#ifndef SHELLPROTOCOL_H
#define SHELLPROTOCOL_H

#include <QtCore>

namespace HuProtocol {

enum class MsgType : quint32 {
    // ── Shell → Module ──────────────────────────────
    ShowModule          = 0x0001,   // payload: qint32 x, y, w, h
    HideModule          = 0x0002,   // no payload
    GearStateUpdate     = 0x0010,   // payload: quint8 gear (GearState enum value)
    VehicleSpeedUpdate  = 0x0011,   // payload: float speed_kmh
    BatteryUpdate       = 0x0012,   // payload: float voltage, float percent
    IpcStatusUpdate     = 0x0013,   // payload: quint8 connected (0=disconnected, 1=connected)
    ShellShutdown       = 0x00FF,   // no payload

    // ── Module → Shell ──────────────────────────────
    ModuleReady         = 0x1001,   // payload: quint32 pid
    GearChangeRequest   = 0x1010,   // payload: quint8 gear, quint8 source
    AmbientColorSet     = 0x1020,   // payload: quint8 r, g, b, brightness
    AmbientOff          = 0x1021,   // no payload
    SettingsChanged     = 0x1030,   // payload: QVariantMap (QDataStream)
};

/** 프레임 하나를 QByteArray로 인코딩 */
QByteArray encodeFrame(MsgType type, const QByteArray &payload = {});

/**
 * 수신 버퍼에서 완성된 프레임 하나를 파싱.
 * @return 완성 프레임이 있으면 true; bytesConsumed에 소비한 바이트 수 반환
 */
bool decodeFrame(const QByteArray &buf,
                 MsgType &outType, QByteArray &outPayload,
                 int &bytesConsumed);

} // namespace HuProtocol

#endif // SHELLPROTOCOL_H
