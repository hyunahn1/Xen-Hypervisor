/**
 * @file VSomeIPClient.h
 * @brief VSOMEIP implementation for Head Unit ↔ Instrument Cluster IPC
 * Subscribe: 0x8001 speed, 0x8002 gear, 0x8003 battery
 * Publish: 0x8002 gear (on touch)
 * Service: 0x1234, Instance: 0x0001
 * Config: /etc/vsomeip/vsomeip_headunit.json
 * @author Ahn Hyunjun
 * @date 2026-02-20
 */

#ifndef VSOMEIPCLIENT_H
#define VSOMEIPCLIENT_H

#include "IVehicleDataProvider.h"

#ifdef HU_HAS_VSOMEIP
#include <vsomeip/vsomeip.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#endif

/**
 * @class VSomeIPClient
 * @brief VSOMEIP client for Head Unit
 *        Subscribes to speed/gear/battery from Cluster
 *        Publishes gear changes to Cluster (on touch)
 */
class VSomeIPClient : public IVehicleDataProvider
{
    Q_OBJECT

public:
    explicit VSomeIPClient(QObject *parent = nullptr);
    ~VSomeIPClient() override;

    float speed() const override;
    GearState gear() const override;
    float batteryVoltage() const override;
    float batteryPercent() const override;
    bool isConnected() const override;

    void publishGear(GearState gear);

private:
    float m_speed;
    GearState m_gear;
    float m_batteryVoltage;
    float m_batteryPercent;
    bool m_connected;

#ifdef HU_HAS_VSOMEIP
    std::shared_ptr<vsomeip::application> m_app;
    std::mutex m_mutex;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_registered{false};
    std::atomic<bool> m_serviceAvailable{false};  // IC 서비스(0x1234)가 실제로 올라왔는가
    void onState(vsomeip::state_type_e state);
    void onAvailability(vsomeip::service_t service, vsomeip::instance_t instance, bool available);
#endif
};

#endif // VSOMEIPCLIENT_H
