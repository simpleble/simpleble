#pragma once

#include <simpleble/Types.h>

#include "AdapterBase.h"
#include "core/DeviceCore.h"
#include "core/SimulatorUtils.h"

#include <kvn_threadrunner.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace SimpleBLE::Simulation::Internal {

class EnvironmentCore;

/** Connection policy of a simulated adapter acting as the central. */
struct CentralPolicy {
    Clock::duration connection_interval = std::chrono::milliseconds(30);
    Clock::duration accepted_interval_min = std::chrono::microseconds(7500);
    Clock::duration accepted_interval_max = std::chrono::seconds(4);
    Clock::duration max_event_length = std::chrono::microseconds(3750);
    Clock::duration supervision_timeout = std::chrono::seconds(4);
    uint16_t mtu = 517;
    uint16_t max_data_length = 251;
};

}  // namespace SimpleBLE::Simulation::Internal

namespace SimpleBLE {

class PeripheralSimulator;

/**
 * Central-side view of a simulated adapter.
 *
 * User callbacks run on this adapter's executor, like the callback thread of
 * an operating system's Bluetooth stack.
 */
class AdapterSimulator : public AdapterBase, public std::enable_shared_from_this<AdapterSimulator> {
  public:
    AdapterSimulator(std::weak_ptr<Simulation::Internal::EnvironmentCore> environment, std::string identifier,
                     BluetoothAddress address);
    virtual ~AdapterSimulator();

    void* underlying() const override;

    std::string identifier() override;
    BluetoothAddress address() override;

    void power_on() override;
    void power_off() override;
    bool is_powered() override;

    void scan_start() override;
    void scan_stop() override;
    void scan_for(int timeout_ms) override;
    bool scan_is_active() override;
    std::vector<std::shared_ptr<PeripheralBase>> scan_get_results() override;

    std::vector<std::shared_ptr<PeripheralBase>> get_paired_peripherals() override;
    std::vector<std::shared_ptr<PeripheralBase>> get_connected_peripherals() override;
    std::shared_ptr<Local::PeripheralBase> create_local_peripheral() override;

    bool bluetooth_enabled() override;

    // Simulation

    /** The simulator adapter behind a Simulation::Adapter handle. */
    static AdapterSimulator& from(const std::shared_ptr<AdapterBase>& adapter);

    Simulation::Internal::CentralPolicy policy() const;
    void update_policy(const std::function<void(Simulation::Internal::CentralPolicy&)>& change);
    std::shared_ptr<kvn::thread_runner> executor() const;
    /** Runs on this adapter's executor. */
    void receive_advertisement(const Simulation::Internal::Advertisement& advertisement,
                               std::weak_ptr<Simulation::Internal::DeviceCore> device);
    void shutdown();

  private:
    std::weak_ptr<Simulation::Internal::EnvironmentCore> environment_;
    const std::string identifier_;
    const BluetoothAddress address_;
    std::shared_ptr<kvn::thread_runner> executor_;

    std::atomic_bool powered_{true};
    std::atomic_bool scanning_{false};

    mutable std::mutex policy_mutex_;
    Simulation::Internal::CentralPolicy policy_;

    std::mutex mutex_;
    std::map<BluetoothAddress, std::shared_ptr<PeripheralSimulator>> peripherals_;
    std::set<BluetoothAddress> seen_;
};

}  // namespace SimpleBLE
