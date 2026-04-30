/**
 * @file VSomeIPClient.cpp
 * @brief VSOMEIP client - gear publish to Instrument Cluster
 * - vsomeip::runtime::get()->create_application()
 * - request_service(0x1234, 0x0001)
 * - register_availability_handler: IC 서비스가 실제로 올라왔는지 확인
 * - send(request) on gear touch (only when service available)
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#include "VSomeIPClient.h"
#include <QDebug>
#include <QMetaObject>

#ifdef HU_HAS_VSOMEIP
#include <cstring>
#include <set>
#include <vector>

namespace {
constexpr vsomeip::service_t      kServiceId         = 0x1234;
constexpr vsomeip::instance_t     kInstanceId        = 0x0001;
constexpr vsomeip::method_t       kGearMethodId      = 0x8002;
constexpr vsomeip::event_t        kSpeedEventId      = 0x8001;
constexpr vsomeip::eventgroup_t   kSpeedEventGroupId = 0x0001;
}
#endif

VSomeIPClient::VSomeIPClient(QObject *parent)
    : IVehicleDataProvider(parent)
    , m_speed(0.0f)
    , m_gear(GearState::P)
    , m_batteryVoltage(7.8f)
    , m_batteryPercent(85.0f)
    , m_connected(false)
{
#ifdef HU_HAS_VSOMEIP
    m_app = vsomeip::runtime::get()->create_application("HeadUnit");
    if (!m_app) {
        qWarning() << "[VSomeIP] Failed to create application";
        return;
    }
    m_app->register_state_handler([this](vsomeip::state_type_e state) {
        onState(state);
    });
    m_running = true;
    m_worker = std::thread([this]() {
        if (!m_app->init()) {
            qWarning() << "[VSomeIP] app init failed";
            return;
        }
        m_app->start();
    });
#endif
}

VSomeIPClient::~VSomeIPClient()
{
#ifdef HU_HAS_VSOMEIP
    m_running = false;
    if (m_app) {
        m_app->stop();
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
#endif
}

float VSomeIPClient::speed() const { return m_speed; }
GearState VSomeIPClient::gear() const { return m_gear; }
float VSomeIPClient::batteryVoltage() const { return m_batteryVoltage; }
float VSomeIPClient::batteryPercent() const { return m_batteryPercent; }
bool VSomeIPClient::isConnected() const { return m_connected; }

void VSomeIPClient::publishGear(GearState gear)
{
    m_gear = gear;
    static const char *names[] = {"P", "R", "N", "D"};
    qDebug() << "[VSomeIP] publishGear:" << names[static_cast<int>(gear)];
#ifdef HU_HAS_VSOMEIP
    if (!m_app || !m_registered.load()) {
        qWarning() << "[VSomeIP] publishGear skipped: not registered with routing manager";
        return;
    }
    if (!m_serviceAvailable.load()) {
        qWarning() << "[VSomeIP] publishGear skipped: IC service 0x1234 not available yet"
                   << "(Instrument Cluster가 먼저 실행됐는지 확인하세요)";
        return;
    }
    auto payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(1);
    data[0] = static_cast<vsomeip::byte_t>(gear);
    payload->set_data(data);

    auto request = vsomeip::runtime::get()->create_request();
    request->set_service(kServiceId);
    request->set_instance(kInstanceId);
    request->set_method(kGearMethodId);
    request->set_payload(payload);
    m_app->send(request);
    qDebug() << "[VSomeIP] gear request sent ->" << names[static_cast<int>(gear)];
#endif
}

#ifdef HU_HAS_VSOMEIP
void VSomeIPClient::onState(vsomeip::state_type_e state)
{
    if (!m_app) {
        return;
    }
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        m_registered = true;
        qDebug() << "[VSomeIP] Registered with routing manager. Requesting service 0x1234...";

        // IC 서비스 0x1234가 올라왔는지 감시 등록
        m_app->register_availability_handler(kServiceId, kInstanceId,
            [this](vsomeip::service_t s, vsomeip::instance_t i, bool available) {
                (void)s; (void)i;
                onAvailability(s, i, available);
            });

        // Subscribe to speed event (0x8001) from IC
        std::set<vsomeip::eventgroup_t> groups = {kSpeedEventGroupId};
        m_app->request_event(kServiceId, kInstanceId, kSpeedEventId, groups);
        m_app->register_message_handler(kServiceId, kInstanceId, kSpeedEventId,
            [this](const std::shared_ptr<vsomeip::message> &msg) {
                if (!msg) return;
                auto pl = msg->get_payload();
                if (!pl || pl->get_length() < 4) return;
                float kmh = 0.0f;
                std::memcpy(&kmh, pl->get_data(), 4);
                m_speed = kmh;
                QMetaObject::invokeMethod(this, [this, kmh]() {
                    emit speedChanged(kmh);
                }, Qt::QueuedConnection);
            });
        m_app->subscribe(kServiceId, kInstanceId, kSpeedEventGroupId);

        m_app->request_service(kServiceId, kInstanceId);
        return;
    }
    // ST_DEREGISTERED
    m_registered = false;
    m_serviceAvailable = false;
    m_connected = false;
    qWarning() << "[VSomeIP] Deregistered from routing manager";
}

void VSomeIPClient::onAvailability(vsomeip::service_t /*service*/,
                                   vsomeip::instance_t /*instance*/,
                                   bool available)
{
    m_serviceAvailable = available;
    m_connected = available;
    if (available) {
        qDebug() << "[VSomeIP] IC service 0x1234 is NOW AVAILABLE - gear IPC ready";
    } else {
        qWarning() << "[VSomeIP] IC service 0x1234 UNAVAILABLE - Instrument Cluster 연결 끊김";
    }
    // onAvailability runs on a vsomeip thread — emit signal on Qt main thread.
    QMetaObject::invokeMethod(this, [this, available]() {
        emit connectionStatusChanged(available);
    }, Qt::QueuedConnection);
}
#endif
