#ifndef SOCKETCANPDCPROVIDER_H
#define SOCKETCANPDCPROVIDER_H

#include "IPdcSensorProvider.h"

#include <QSocketNotifier>
#include <QString>

class SocketCanPdcProvider : public IPdcSensorProvider
{
    Q_OBJECT

public:
    explicit SocketCanPdcProvider(const QString &interfaceName = QStringLiteral("can0"),
                                  QObject *parent = nullptr);
    ~SocketCanPdcProvider() override;

    void start() override;
    void stop() override;
    bool isAvailable() const override { return m_socket >= 0; }

private slots:
    void onCanReadyRead();

private:
    bool openSocket();
    void closeSocket();
    void emitUniformRearDistance(float distanceCm);
    void emitFourRearDistances(float rl, float rml, float rmr, float rr);

    QString m_interfaceName;
    int m_socket = -1;
    QSocketNotifier *m_notifier = nullptr;
};

#endif // SOCKETCANPDCPROVIDER_H
