/**
 * @file VSomeIPGearReceiver.cpp
 * @brief VSOMEIP gear receiver for Instrument Cluster
 * @author Ahn Hyunjun
 * @date 2026-02-26
 */

#include "VSomeIPGearReceiver.h"
#include <QDebug>

#ifdef IC_HAS_VSOMEIP
#include <cstring>
#include <set>
#include <vector>

namespace {
constexpr vsomeip::service_t kServiceId = 0x1234;
constexpr vsomeip::instance_t kInstanceId = 0x0001;
constexpr vsomeip::method_t kGearMethodId = 0x8002;
}
#endif

VSomeIPGearReceiver::VSomeIPGearReceiver(QObject *parent)
    : QObject(parent)
{
#ifdef IC_HAS_VSOMEIP
    m_app = vsomeip::runtime::get()->create_application("InstrumentCluster");
    if (!m_app) {
        return;
    }
    m_app->register_state_handler([this](vsomeip::state_type_e state) {
        onState(state);
    });
    m_app->register_message_handler(kServiceId, kInstanceId, kGearMethodId,
                                    [this](const std::shared_ptr<vsomeip::message> &msg) {
        onMessage(msg);
    });

    m_running = true;
    m_worker = std::thread([this]() {
        if (!m_app->init()) {
            return;
        }
        m_app->start();
    });
#endif
}

VSomeIPGearReceiver::~VSomeIPGearReceiver()
{
#ifdef IC_HAS_VSOMEIP
    m_running = false;
    if (m_app) {
        m_app->stop();
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
#endif
}

#ifdef IC_HAS_VSOMEIP
void VSomeIPGearReceiver::onState(vsomeip::state_type_e state)
{
    if (!m_app) {
        return;
    }
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        m_registered = true;
        // Offer speed event so HU can subscribe
        std::set<vsomeip::eventgroup_t> groups = {kSpeedEventGroupId};
        m_app->offer_event(kServiceId, kInstanceId, kSpeedEventId, groups,
                           vsomeip::event_type_e::ET_FIELD);
        m_app->offer_service(kServiceId, kInstanceId);
        return;
    }
    m_registered = false;
}

void VSomeIPGearReceiver::sendSpeed(float kmh)
{
#ifdef IC_HAS_VSOMEIP
    if (!m_app || !m_registered.load()) {
        return;
    }
    auto payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data(4);
    std::memcpy(data.data(), &kmh, 4);
    payload->set_data(data);
    m_app->notify(kServiceId, kInstanceId, kSpeedEventId, payload);
#else
    (void)kmh;
#endif
}

void VSomeIPGearReceiver::onMessage(const std::shared_ptr<vsomeip::message> &msg)
{
    if (!msg) {
        return;
    }
    const auto payload = msg->get_payload();
    if (!payload || payload->get_length() < 1) {
        return;
    }
    const unsigned char *data = payload->get_data();
    if (!data) {
        return;
    }

    QString gear = "P";
    switch (data[0]) {
    case 0:
        gear = "P";
        break;
    case 1:
        gear = "R";
        break;
    case 2:
        gear = "N";
        break;
    case 3:
        gear = "D";
        break;
    default:
        gear = "P";
        break;
    }
    qDebug() << "[VSomeIP] gearReceived:" << gear;
    emit gearReceived(gear);
}
#endif
