#ifndef MOCKPDCSENSORPROVIDER_H
#define MOCKPDCSENSORPROVIDER_H

#include "IPdcSensorProvider.h"

#include <QTimer>

class MockPdcSensorProvider : public IPdcSensorProvider
{
    Q_OBJECT

public:
    explicit MockPdcSensorProvider(QObject *parent = nullptr);

    void start() override;
    void stop() override;
    bool isAvailable() const override { return true; }

private slots:
    void publishNextFrame();

private:
    QTimer m_timer;
    int m_phase = 0;
};

#endif // MOCKPDCSENSORPROVIDER_H
