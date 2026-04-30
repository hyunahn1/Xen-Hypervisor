/**
 * @file CallService.h
 * @brief Call 모듈 서비스 레이어 — 통화 상태 관리
 */
#ifndef CALLSERVICE_H
#define CALLSERVICE_H

#include <QObject>

class CallService : public QObject
{
    Q_OBJECT
public:
    explicit CallService(QObject *parent = nullptr);
    bool isInCall() const { return m_inCall; }

public slots:
    void startCall();
    void endCall();

signals:
    void callStateChanged(bool inCall);

private:
    bool m_inCall = false;
};
#endif // CALLSERVICE_H
