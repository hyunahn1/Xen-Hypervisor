#include "CallService.h"

CallService::CallService(QObject *parent) : QObject(parent) {}
void CallService::startCall() { m_inCall = true;  emit callStateChanged(true); }
void CallService::endCall()   { m_inCall = false; emit callStateChanged(false); }
