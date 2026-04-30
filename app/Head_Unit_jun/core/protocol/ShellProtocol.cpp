/**
 * @file ShellProtocol.cpp
 */

#include "ShellProtocol.h"

namespace HuProtocol {

static constexpr int HEADER_SIZE = sizeof(quint32) * 2; // length + type

QByteArray encodeFrame(MsgType type, const QByteArray &payload)
{
    QByteArray frame;
    QDataStream ds(&frame, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint32>(payload.size());
    ds << static_cast<quint32>(type);
    frame.append(payload);
    return frame;
}

bool decodeFrame(const QByteArray &buf,
                 MsgType &outType, QByteArray &outPayload,
                 int &bytesConsumed)
{
    if (buf.size() < HEADER_SIZE)
        return false;

    QDataStream ds(buf);
    ds.setByteOrder(QDataStream::BigEndian);

    quint32 payloadLen = 0;
    quint32 typeRaw    = 0;
    ds >> payloadLen >> typeRaw;

    const int totalSize = HEADER_SIZE + static_cast<int>(payloadLen);
    if (buf.size() < totalSize)
        return false;

    outType        = static_cast<MsgType>(typeRaw);
    outPayload     = buf.mid(HEADER_SIZE, static_cast<int>(payloadLen));
    bytesConsumed  = totalSize;
    return true;
}

} // namespace HuProtocol
